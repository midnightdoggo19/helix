# Debugging Guide

How to find and fix bugs in HelixOS.

## The three log channels

1. **VGA** — what you see on the screen
2. **Serial** — duplicated to host stdout via `-serial stdio`
3. **QEMU monitor** — debug commands sent to QEMU itself, not the guest

Every `kprintf` fans out to **both VGA and serial**, so anything that screens off (panic banner, scrolling output) is also in `/tmp/serial.log` or your terminal.

## Boot-time failures

### Triple fault (instant reboot loop)

QEMU's hint: you'll see your kernel run for a few ms then GRUB reappear. Add `-no-reboot` to QEMU args so it freezes instead of restarting.

Causes, in order of likelihood:

1. **Bad GDT** — selectors don't match what `gdt_flush` reloads
2. **Bad IDT** — `iret` returns to garbage because EFLAGS/CS got pushed wrong
3. **Page fault before paging is set up** — touching above 4 MiB before identity map
4. **Stack corruption** — most often from a missing `static` causing globals to live in `.bss` with wrong size

Diagnostic:

```bash
qemu-system-i386 -cdrom build/helix.iso -m 128M -no-reboot -d int,cpu_reset
```

The `-d int,cpu_reset` flag logs every interrupt and every reset reason to stderr. Look for the **first** non-IRQ-0 entry.

### Hang after a specific `[ok]` line

The most useful information is *which* `[ok]` printed last. That tells you which init step succeeded and which crashed. Then add `kprintf` debug before/after the suspect line and rebuild.

### Garbled VGA output

If the screen shows random characters: the VGA buffer is probably being written by something other than `vga_putc`. Run `grep -rn '0xB8000' src/` — only `drivers/vga/vga.c` should mention it.

## Runtime failures

### Kernel panic with red banner

The banner says:

```
[ KERNEL PANIC ] <message>
System halted.
```

The message tells you the cause. If it includes `KASSERT failed: <expr> (at <file>:<line>)`, go look at that line.

For more context, add a backtrace before the panic. Naive 4-line backtrace using frame pointers:

```c
void dump_backtrace(void) {
    uint32_t *bp;
    __asm__ volatile("mov %%ebp, %0" : "=r"(bp));
    for (int i = 0; i < 16 && bp; i++) {
        kprintf("  #%d  eip=0x%08x\n", i, bp[1]);
        bp = (uint32_t *)bp[0];
    }
}
```

Compile with `-fno-omit-frame-pointer` (already in our Makefile).

### Page fault panic

Banner shows the faulting virtual address and decoded error bits:

```
[ PAGE FAULT ] virt=0x40000000  eip=0x001022b6  err=0x2
  cause: not-present, write, kernel
```

Triage:

| `virt` is... | Likely cause |
|---|---|
| 0x00000000 | NULL dereference |
| Close to a known pointer | Off-by-one or uninitialized field |
| In the unmapped range (8 MiB ... 12 MiB) | Forgot to `vmm_alloc_region` |
| Above the heap | Wild pointer or corrupted task stack |
| Inside the kernel (`0x100000+`) with err bit 3 | Reserved bit in PDE/PTE — paging corruption |

The `eip` tells you what code did the access. Look it up:

```bash
addr2line -e build/helix.bin 0x001022b6
# src/memory/heap.c:42
```

### Hang (no output, no panic, no crash)

The kernel is alive but stuck. Press `Ctrl-A then C` in QEMU and run:

```
(qemu) info registers
```

Look at EIP. Then `addr2line -e build/helix.bin <eip>` to see where you are.

Common causes:

- Infinite loop with `cpu_halt` and no IRQs (PIC masks set wrong)
- Mutex deadlock (run `info pic` — if no IRQs are pending, see if anything's waiting on a held mutex)
- Stuck in the panic path before the banner printed (try `make debug` and break on `kpanic`)

### Keyboard doesn't respond

Already documented as [ADR-0005](design/adr/0005-eoi-before-handler.md). The PIC's in-service register is blocking. Sanity-check:

```
(qemu) info pic
pic0: irr=00 imr=fc isr=01
```

`isr=01` means IRQ 0 still in service — the bug this ADR fixes.

### Scheduler doesn't preempt

Symptoms: one task runs forever, others never start, despite the PIT ticking.

- Check EFLAGS at every `popfd` site. If IF=0, no IRQs fire.
- Check task's initial stack frame ordering — [ADR-0006](design/adr/0006-context-switch-frame-layout.md).
- Print `cpu_read_eflags()` inside a worker task — should have bit 9 (IF) set.

### Heap corruption

If `kfree` panics with "corrupted block at <addr>":

1. The block immediately *before* the freed one wrote past its end
2. Or a use-after-free corrupted the next block's header

Add `kprintf` of every `kmalloc/kfree` with sizes and pointers. Replay the sequence until the magic byte breaks.

A future improvement: red-zones (pad each allocation with extra bytes, fill with `0xCC`, check on free).

## GDB inside QEMU

### Setup

```bash
make debug
# (waits at boot.asm's _start, listens on :1234)
```

In a second terminal:

```bash
i686-elf-gdb build/helix.bin
(gdb) target remote :1234
(gdb) break kmain
(gdb) continue
```

### Useful commands

| Command | Effect |
|---|---|
| `break <fn>` | Symbolic breakpoint |
| `break <file>:<line>` | Line breakpoint |
| `hbreak` | Hardware breakpoint (useful in early boot) |
| `step` | Step into |
| `next` | Step over |
| `finish` | Run to end of current function |
| `info reg` | All registers |
| `info reg eax` | One register |
| `print/x var` | Print variable in hex |
| `print *task_list` | Dereference a pointer |
| `display/i $pc` | Show next instruction each step |
| `x/16wx 0x100000` | Hex dump 16 words |
| `x/i 0x001022b6` | Disassemble at address |

### Inspecting the task list

After `task_init`:

```
(gdb) p init_task
(gdb) p init_task.next->name
(gdb) p init_task.next->state
```

`task_state_t` is an enum — GDB will show `TASK_RUNNING` etc.

## QEMU flags cheat sheet

| Flag | Effect |
|---|---|
| `-cdrom build/helix.iso` | Boot from our ISO |
| `-m 128M` | RAM size |
| `-serial stdio` | Serial → host terminal |
| `-display none` | No graphical window (CI/headless) |
| `-monitor stdio` | QEMU monitor on host terminal (incompatible with `-serial stdio`) |
| `-s` | GDB server on :1234 |
| `-S` | Pause at start |
| `-no-reboot` | Halt instead of triple-fault-reset |
| `-d int,cpu_reset,page` | Log interrupts, resets, and page-table walks |
| `-D /tmp/qemu.log` | Send `-d` output to a file |

## When all else fails

1. `git diff main` — what changed?
2. `git bisect start; git bisect bad; git bisect good <last-known-good>` — find the commit
3. Take a walk and come back. Real story: the EFLAGS bug from ADR-0006 was diagnosed after stepping away for ten minutes.
