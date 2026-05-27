# Architecture Overview

This document explains the high-level shape of HelixOS — what kind of OS it is, what's in the box, and how the pieces talk to each other.

## What HelixOS is

A **monolithic, single-CPU, kernel-thread-only operating system** targeting **32-bit x86 (i386)**. It boots via the Multiboot1 protocol (GRUB), runs entirely in ring 0, and offers an interactive shell over the VGA console. There is no user mode yet — every "process" is a kernel thread. User-mode separation is a v0.2 goal (the GDT already has user-segment slots; we just don't switch CPL).

This puts HelixOS roughly between **xv6** (which has user mode and disk I/O but no real scheduler) and **a from-scratch tutorial kernel** (which usually stops after IDT setup). The deliberate scope: enough subsystems to demonstrate real OS engineering, few enough that one author can finish.

## Design principles

1. **Layered, not coupled.** Each subsystem depends only on the ones below it. `drivers/` know about `arch/`, but `memory/` doesn't know about `drivers/keyboard`. This is enforced by include discipline, not a build-system gate, but it's reviewable.
2. **Architecture quarantine.** Everything x86-specific lives in `src/arch/i386/`. Porting to ARM/RISC-V or to x86_64 should mean rewriting only that directory.
3. **One way to do each thing.** One PMM. One scheduler. One filesystem. We add alternatives only when there's a concrete reason. This avoids the "I built an `IPageTableManager` interface and then never wrote a second implementation" antipattern.
4. **Fail loud.** Any unexpected condition panics with a red banner showing the file + line. There is no `errno` smuggling in the kernel.
5. **Reviewable boot.** Every initialization step prints `[ok] <subsystem> initialized` so the boot log itself is a contract.

## Layered architecture

```
┌──────────────────────────────────────────────────────────────┐
│  Shell (kernel thread, calls syscalls)                       │  src/shell/
├──────────────────────────────────────────────────────────────┤
│  System call gate (INT 0x80) — 9 syscalls                    │  src/syscalls/
├──────────────────────────────────────────────────────────────┤
│  Process / scheduler / sync     File system (VFS + RAMFS)    │  src/process/  src/fs/
├──────────────────────────────────────────────────────────────┤
│  Memory: PMM → paging → VMM → heap                           │  src/memory/
├──────────────────────────────────────────────────────────────┤
│  Drivers: VGA, serial, PIT, keyboard                         │  src/drivers/
├──────────────────────────────────────────────────────────────┤
│  HAL: GDT, IDT, ISR/IRQ, PIC, port I/O                       │  src/arch/i386/
├──────────────────────────────────────────────────────────────┤
│  Boot: Multiboot1 entry + linker script                      │  src/boot/
└──────────────────────────────────────────────────────────────┘
                            ↑
                          x86 hardware (via QEMU or real PC)
```

A box only calls *down*. Drivers may use the heap (going down past memory) but the heap never calls into a driver. This linearity is what makes the kernel teachable.

## Bring-up order

The order in `kmain()` is deliberate — each step needs everything before it.

1. **Serial** — debug log channel. Comes first so any later failure can be reported even before the VGA driver works.
2. **VGA** — primary console. Must come before any `kprintf`.
3. **GDT** — replace GRUB's transient GDT with our own. Required before anything that depends on segment selectors (which is everything).
4. **IDT** — empty 256-entry table, ready to receive gates.
5. **ISR install** — vectors 0..31 wired to the C exception handler.
6. **IRQ install** — vectors 32..47 wired to the IRQ dispatcher.
7. **PIC** — remap legacy 8259 from 0x00..0x0F to 0x20..0x2F so IRQ lines don't collide with CPU exceptions.
8. **PMM** — parse the Multiboot memory map, build a bitmap of free frames.
9. **Paging** — install a kernel page directory, identity-map 8 MiB, flip CR0.PG.
10. **VMM** — install the page-fault handler, expose `vmm_alloc_region`.
11. **Heap** — `vmm_alloc_region` a 1 MiB region for `kmalloc`/`kfree`.
12. **STI** — interrupts on. From this moment the PIT can preempt.
13. **PIT** — 100 Hz tick, IRQ 0 unmasked. Drives the scheduler.
14. **Keyboard** — IRQ 1 unmasked, ring buffer ready.
15. **Task / scheduler** — install the init task PCB, scheduler online.
16. **Syscalls** — install `INT 0x80` gate (DPL=3 so future user-mode code can call it).
17. **RAMFS** — build root, seed `/etc/motd` and `/README`.
18. **Spawn shell.** Init halts in an idle loop.

Any failure in the first 7 stages is fatal (we don't even have a heap to retry from). Stages 8 and later can panic gracefully because the console is up.

## Data flow examples

### Typing `ls` at the shell

```
keyboard hardware
   ↓
8259 PIC raises IRQ 1
   ↓
irq1 stub (irq_stubs.asm)  → cli, save state, push 0, push 33
   ↓
irq_common  →  pic_send_eoi(1)  →  irq_dispatch(regs)
   ↓
keyboard.c handler  →  inb(0x60)  →  scancode lookup  →  buffer_push('l')
   ↓
shell task wakes from cpu_halt (in kbd_getchar)
   ↓
sh_readline accumulates 'l','s',\n  →  echoes via sys_write
   ↓
cmd_dispatch("ls", ...)
   ↓
c_ls  →  vfs_lookup("/")  →  walks children  →  sys_write for each
```

### Allocating memory inside the shell

```
shell: kmalloc(sizeof(thing))     // we use it via VFS internally
   ↓
heap_t free-list walk
   ↓
if no fit:  pmm_alloc_frame()  → vmm_map() → enlarge region   (not yet — fixed-size heap in v0.1)
otherwise:  split block, mark used, return payload pointer
```

## Memory map at runtime

```
0x00000000 ─┐
            │  legacy BIOS / IVT / VGA buffers      (reserved)
0x000A0000 ─┤
            │  VGA text-mode framebuffer @ 0xB8000  (used by drivers/vga)
0x00100000 ─┤
            │  kernel image (.multiboot/.text/.rodata/.data/.bss)
kernel_end ─┤
            │  PMM bitmap (4 KiB)
            │  ...
0x00C00000 ─┤
            │  kernel heap (1 MiB)
0x00D00000 ─┤
            │  task stacks (allocated by kmalloc)
            │
0x07FFFFFF ─┘  (top of 128 MiB QEMU default)
```

Everything ≤ 8 MiB is identity-mapped (PA == VA). Higher addresses are unmapped — touching them produces a clean page fault that the VMM handler reports.

## Concurrency model

Single CPU, preemption via PIT.

- **Atomic region** = anything between `cpu_cli` and `cpu_sti`. No other code runs.
- **Spinlock** = save EFLAGS, `cli`. Lock-free in the literal sense (single CPU) but still useful as an explicit acknowledgment that "this critical section disables preemption."
- **Mutex** = a `volatile bool` + yielding loop. Cheap, fair-ish, deadlock-free as long as locks are taken in consistent order.
- **Semaphore** = counting variant of the mutex.

There are no async signals, no IPI, no atomic builtins. The model is deliberately *minimal* — adding SMP is a separate effort that we can scope in v0.3.

## What deliberately isn't here

| Not in v0.1 | Why |
|---|---|
| User mode (ring 3) | The GDT has the slots, but we never `iretd` to ring 3. Reaching it cleanly needs a syscall path that switches stacks via TSS — out of scope for the demo. |
| `fork` / `exec` | No user mode means no process model in the POSIX sense. |
| ELF loader | Nothing to load yet. |
| Disk driver | RAMFS is enough to demonstrate the FS layer. |
| Network | Same reasoning — orthogonal complexity. |
| SMP | Single CPU keeps the concurrency model legible; SMP makes every other subsystem harder to explain. |

These are all on the [ROADMAP](../../ROADMAP.md).
