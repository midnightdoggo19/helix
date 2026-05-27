# ADR-0006: Initial task-stack frame places EFLAGS at the bottom

**Status**: Accepted
**Date**: 2026-05-26

## Context

When `task_create` allocates a new task, it has to pre-populate the kernel stack so that the very first `context_switch` to this task does the right thing. The `context_switch` routine (in `context_switch.asm`) pops registers from the new task's stack in a fixed order:

```nasm
context_switch:
    ...
    mov   esp, [esp + 28]    ; ESP <- new_esp
    popfd                    ; restore EFLAGS  ← first pop
    pop   edi
    pop   esi
    pop   ebx
    pop   ebp
    ret                      ; pops return address
```

Since the stack grows downward, the *first* thing popped sits at the *lowest* address of the saved frame.

## Decision

The initial frame in `task_create` is built as (low address → high address):

```
EFLAGS  = 0x202   ← popfd reads this first
EDI     = 0
ESI     = 0
EBX     = 0
EBP     = 0
task_trampoline   ← ret jumps here
entry             ← trampoline's `ret` jumps here
task_exit         ← if entry ever returns
```

## The bug this ADR documents

The original code had the order wrong — EFLAGS in the middle of the frame:

```c
/* WRONG */
*--sp = task_exit;
*--sp = entry;
*--sp = task_trampoline;
*--sp = 0x202;     /* EFLAGS  -- WRONG SLOT */
*--sp = 0;         /* ebp */
*--sp = 0;         /* ebx */
*--sp = 0;         /* esi */
*--sp = 0;         /* edi */
```

When `context_switch` ran `popfd` for a new task, it read EDI's slot — which contained 0. EFLAGS was zeroed, including the IF bit. Interrupts stayed *disabled* on every newly switched-to task.

**Symptom**: in Phase 3e, three worker tasks were spawned, each printing an iteration counter and calling `task_sleep_ms`. The output showed all 18 iterations (3 tasks × 6 each) completing in ~3 PIT ticks (30 ms instead of the expected ~2 seconds). The sleep wasn't sleeping; the scheduler wasn't preempting; nothing was working.

Diagnosis took longer than the fix:

1. Workers complete in 30 ms instead of 2 s — so they're not waiting
2. The "tick=" log values show they all see roughly the same tick count — so it's not a per-task counter bug, it's a global state issue
3. `task_sleep_ms` sets `state = SLEEPING` and calls `schedule()` — but the task wakes up and continues immediately?
4. `task_wake_due` only flips back to READY when `tick >= wake_tick` — so either tick is racing forward or wake_tick is being set wrong
5. tick is `volatile uint32_t` incremented in the PIT handler — but PIT handler depends on IRQ 0 firing
6. IRQ 0 requires IF=1 — and the task was running with IF=0!
7. Trace: where does IF come from on a freshly switched-to task? → from the popfd in `context_switch` → which reads the bottom of the frame → which contains... whatever we put there.

## Why this is easy to get wrong

Stack convention diagrams are conventionally drawn with "high addresses at top" — but that's backwards from how `push`/`pop` mutate the stack pointer. Stack grows down, so the most recently pushed item is at the *lowest* address. When you read a diagram and think "EFLAGS is pushed last, so it's on top, so it's popped first" — that's correct — but then you have to remember "on top" means "lowest address," not "highest in the diagram."

The fix is to draw the frame in the order things actually get popped, not in the order the C code pushes them. That ordering is now the canonical view used in `task.c`'s file-header comment.

## Consequences

**Positive**

- Preemption works
- Sleep works
- Race conditions in subsequent phases (3f shell input) became diagnosable

**Negative**

- The C code that builds the frame reads "backwards" relative to the popfd-first order. To compensate, `task.c`'s file header explicitly diagrams the final stack with addresses ascending. Every reviewer must check both files together.

## Lessons

1. **EFLAGS must always be saved/restored as a complete pair.** Half-saving causes silent IF clears.
2. **The init frame is the most error-prone code in a kernel.** It's the only place where you simulate the result of a save you never actually did.
3. **Run multi-task demos early.** A single-task kernel can hide context-switch bugs for a long time. As soon as a second task exists, run them concurrently and watch the clock.
