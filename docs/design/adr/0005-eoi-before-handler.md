# ADR-0005: Send PIC EOI before invoking the IRQ handler

**Status**: Accepted
**Date**: 2026-05-26

## Context

The naive IRQ dispatcher acknowledges the 8259 PIC *after* the handler returns:

```c
void irq_dispatch(registers_t *r) {
    uint8_t irq = r->int_no - 32;
    if (irq < 16 && handlers[irq])
        handlers[irq](r);
    pic_send_eoi(irq);    // ← post-handler
}
```

This is what most tutorials show and what HelixOS initially used. It's correct for any handler that *returns*. But the PIT handler doesn't always return through the same path it entered — when `scheduler_tick` decides to preempt, it calls `schedule()` which calls `context_switch()` which leaves the current IRQ frame sitting on the *old* task's kernel stack and resumes execution from a *different* task's stack.

The result: the PIC's "in service" bit for IRQ 0 stays set indefinitely. The PIC's priority logic refuses to deliver any IRQ of equal or lower priority while IRQ 0 is in service — that includes IRQ 1 (keyboard) on the master 8259.

**Concrete symptom**: in Phase 3f, after the first PIT-driven context switch, keystrokes stopped reaching the kernel. The keyboard handler was wired correctly, the IRQ 1 unmask in `kbd_init` had run — but the PIC never delivered IRQ 1 because IRQ 0 still showed as in-service.

## Decision

**Send EOI before the handler is invoked.**

```c
void irq_dispatch(registers_t *r) {
    uint8_t irq = r->int_no - 32;
    pic_send_eoi(irq);                  // ← BEFORE
    if (irq < 16 && handlers[irq])
        handlers[irq](r);
}
```

## Why this is safe

The IRQ stub holds `cli` until `iret`. EOI just acknowledges the PIC — it does **not** unblock further IRQs from being delivered to *this* CPU. The CPU's EFLAGS.IF is still cleared, so:

1. We EOI the PIC. (PIC marks IRQ 0 as no longer in-service.)
2. We invoke the C handler.
3. The handler may schedule and context-switch to another task.
4. That task's first instruction runs with IF=1 (popfd restored that), so IRQs can now be delivered.
5. By then, IRQ 0 (PIT) and IRQ 1 (keyboard) are both eligible — the PIC won't block one because of the other.

The race window where "EOI sent but cli still in effect" is microscopic and benign: the only thing it changes is the PIC's bookkeeping state, which is fine.

## What we ruled out

### "Manually EOI inside the handler before context-switching"

Spreads the EOI responsibility into every IRQ handler that might preempt. Brittle — easy to forget. Rejected.

### "Don't preempt from inside an IRQ"

Defer the schedule to a software-interrupt or to the next yield. This is what some kernels do. But it loses the timer-driven preemption guarantee — a CPU-bound task could starve everything else until it yielded voluntarily. Rejected; the whole point of having a preemptive scheduler is that *no* task can hog the CPU.

### "Send EOI in the IRQ stub assembly"

Putting `pic_send_eoi` in the stub would work but means duplicating the master-vs-slave logic in 16 places. Rejected — keeping the EOI in C makes it inspectable and modifiable.

## Consequences

**Positive**

- Keyboard IRQs work even when the PIT preempts
- Other future IRQ-driven drivers (disk, NIC) will Just Work
- One-line change, no API impact

**Negative**

- Theoretical: if a handler triggers a *new* IRQ 0 before returning, the PIC's reentry detection won't help us. But we hold `cli` the whole time, so this can't actually happen on a single CPU.
- A handler that takes longer than 10 ms (one PIT period) will see the *next* PIT IRQ fire as soon as the current returns — but that's true with or without this fix, and is the correct behavior.

## Discovery story

Caught during Phase 3f acceptance testing. The shell was reading the keyboard via `kbd_getchar`, which calls `cpu_halt` in a loop until `kbd_has_input()` returns true. We watched the boot reach the prompt cleanly, then noticed that typed keystrokes never arrived: no echo, no command execution.

Debug path:

1. Verified the keyboard IRQ handler was being called → it wasn't (no `kprintf` from inside the handler fired)
2. Verified IRQ 1 was unmasked at the PIC → it was (read back IMR = 0xFC)
3. Read the 8259 PIC datasheet's ISR section → noticed in-service blocks lower priority
4. Traced the PIT handler's path → confirmed `scheduler_tick` → `context_switch` skips the post-handler EOI
5. Moved the EOI to before the handler → keystrokes flowed

The fix was a 4-line change; the diagnosis was the whole afternoon.
