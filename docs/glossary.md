# Glossary

Quick definitions of every non-obvious term used in the HelixOS docs. Where a term has a longer treatment elsewhere, the cross-reference is included.

## A

**ABI** — Application Binary Interface. The contract for how functions pass arguments, return values, and use registers. HelixOS follows System V i386 for C functions; the syscall ABI is documented separately in [`api/syscalls.md`](api/syscalls.md).

**ADR** — Architecture Decision Record. Short markdown documents under `docs/design/adr/` that lock down one decision with its context and consequences. See ADR-0001 through ADR-0006.

## B

**BSS** — Block Started by Symbol. The ELF section holding statically-allocated variables that are zero-initialized. The kernel's boot stack lives here.

**Bitmap (PMM)** — The data structure used by the Physical Memory Manager: one bit per 4 KiB frame, set if used. See [ADR-0004](design/adr/0004-bitmap-pmm.md).

## C

**Calling convention** — How arguments and return values move between caller and callee. We use cdecl/System V i386: arguments pushed right-to-left on the stack, return in EAX, caller cleans up, callee preserves EBX/ESI/EDI/EBP.

**Context switch** — Saving one task's CPU state and loading another's. In HelixOS, implemented in `src/process/context_switch.asm`; see [scheduler-design.md](architecture/scheduler-design.md#context-switch).

**CR0, CR2, CR3** — x86 control registers. CR0's PG bit enables paging; CR2 holds the faulting virtual address on a page fault; CR3 points to the active page directory.

**Cross-compiler** — A compiler that targets a system different from the one it runs on. We use `i686-elf-gcc` to build kernel code on Linux/macOS hosts.

## D

**DPL** — Descriptor Privilege Level. The 2-bit privilege field in a GDT/IDT descriptor. 0 = kernel; 3 = user. The `INT 0x80` gate uses DPL=3 so future ring-3 code can call it.

## E

**EFLAGS** — The x86 processor flags register. Bit 9 (IF) enables/disables interrupts. Saved and restored across context switches via `pushfd`/`popfd`.

**EOI** — End of Interrupt. The acknowledgment a handler must send to the 8259 PIC so it'll deliver further IRQs. Timing matters — see [ADR-0005](design/adr/0005-eoi-before-handler.md).

## F

**Freestanding** — A C compilation environment without a hosted libc. The kernel uses `-ffreestanding -nostdlib -fno-builtin` and provides its own `memcpy`, `printf`, etc.

**Frame** (memory) — A 4 KiB chunk of physical memory. Distinct from a "page," which is the virtual-side counterpart. The PMM hands out frames.

**Frame** (stack) — The portion of a stack belonging to one function invocation: arguments, return address, saved registers, locals.

## G

**GDT** — Global Descriptor Table. Defines memory segments for the x86. See [interrupt-handling.md](architecture/interrupt-handling.md#gdt). HelixOS uses 5 entries: null, kernel code, kernel data, user code, user data.

**GRUB** — GRand Unified Bootloader. Loads our kernel via the Multiboot1 protocol. See [ADR-0003](design/adr/0003-multiboot1.md).

## H

**HAL** — Hardware Abstraction Layer. In HelixOS, everything in `src/arch/i386/`.

**Heap** — Dynamically-allocated kernel memory, managed by `kmalloc`/`kfree`. Backed by a free-list allocator with split + coalesce. See [memory-model.md](architecture/memory-model.md#layer-4--kernel-heap).

## I

**Identity map** — A page-table layout where virtual address == physical address. HelixOS identity-maps the first 8 MiB so kernel code can address itself after paging is enabled.

**IDT** — Interrupt Descriptor Table. 256 gate descriptors that the CPU consults when handling exceptions, IRQs, and software interrupts. See [interrupt-handling.md](architecture/interrupt-handling.md#idt).

**IRR / ISR (PIC)** — Interrupt Request Register / In-Service Register. PIC bookkeeping for pending and acknowledged IRQs.

**IRQ** — Interrupt Request. Hardware-driven interrupt, delivered via the 8259 PIC. IRQ 0 is the PIT; IRQ 1 is the keyboard.

**ISR** — In HelixOS docs: Interrupt Service Routine, used here to mean "CPU exception handler" (vectors 0..31). Confusingly, "ISR" is also the PIC's In-Service Register — context disambiguates.

**IST** — Interrupt Stack Table. An x86_64 feature for switching stacks on specific interrupts. Not relevant in 32-bit, listed here so readers don't get confused.

## K

**Kernel space** — Memory and instructions running at ring 0 with full privileges. In v0.1, everything is kernel space.

**`kmalloc` / `kfree`** — Kernel-space allocator. Allocates from the heap region; not the same as the C library `malloc`.

## M

**MMIO** — Memory-Mapped I/O. Devices accessed by reading/writing specific memory addresses. The VGA framebuffer at 0xB8000 is MMIO. (Most of our hardware uses port I/O instead.)

**Multiboot1** — The boot protocol spec used by GRUB. Our kernel embeds a Multiboot1 header so GRUB knows how to load it. See [ADR-0003](design/adr/0003-multiboot1.md).

## N

**`NULL`** — In our code, just `((void*)0)`. We don't dereference it; the page-fault handler catches the attempt.

## P

**Page** — A 4 KiB chunk of virtual address space. Maps to a frame via the page table.

**Page Directory (PD)** — Top-level table in 32-bit paging. 1024 PDEs (page directory entries), each pointing to a page table.

**Page Table (PT)** — Second-level table. 1024 PTEs, each pointing to a physical frame.

**Page fault** — CPU exception (vector 14) raised when paging translation fails. Our handler in `src/memory/vmm.c` decodes the error and panics.

**Paging** — The translation of virtual to physical addresses via page tables. Enabled by setting CR0.PG. See [memory-model.md](architecture/memory-model.md#layer-2--paging).

**PCB** — Process (or task) Control Block. The struct holding a task's state. In HelixOS: `task_t`, 40 bytes.

**PIC** — Programmable Interrupt Controller. We use the legacy 8259 PIC (master + slave). See [interrupt-handling.md](architecture/interrupt-handling.md#the-8259-pic).

**PIT** — Programmable Interval Timer. Channel 0 wired to IRQ 0. Configured at 100 Hz. Drives the scheduler.

**PMM** — Physical Memory Manager. Owns every 4 KiB frame. Bitmap-based. See [ADR-0004](design/adr/0004-bitmap-pmm.md).

**Port I/O** — Communication with devices through the dedicated x86 I/O ports (instructions `IN`/`OUT`). Distinct address space from RAM.

**Preemption** — Forcibly interrupting a running task to let another run. Driven by the PIT IRQ in HelixOS.

## R

**RAMFS** — A filesystem implementation that lives entirely in RAM. Our only filesystem in v0.1.

**Ring** — x86 privilege level (0..3). Ring 0 is kernel; ring 3 is user. HelixOS only uses ring 0 in v0.1.

**Round Robin (RR)** — A scheduling discipline where tasks take turns for equal time slices. HelixOS's default and only policy.

## S

**Scheduler** — The component that picks which task runs next. See [scheduler-design.md](architecture/scheduler-design.md).

**Spinlock** — Conceptually, a busy-wait lock. On HelixOS's single CPU it's implemented as save-and-cli; the name is kept for SMP-friendliness.

**Stack frame** — One function's portion of the stack. Includes return address, saved callee-saved registers, locals.

**Syscall** — System call. The interface user code (today: kernel threads; tomorrow: user processes) uses to request kernel services. Vector 0x80 in HelixOS.

## T

**Task** — A schedulable execution context. In v0.1, all tasks are kernel threads. Each has a PCB, an 8 KiB stack, and a state (NEW/READY/RUNNING/SLEEPING/ZOMBIE).

**Tick** — One PIT period (10 ms). The scheduler's time unit.

**TLB** — Translation Lookaside Buffer. CPU's cache of recent virtual-to-physical translations. Invalidated with `invlpg` when a PTE changes.

**Triple fault** — When an exception fires while handling a double fault, the CPU resets. Useful for `reboot`; otherwise a sign of broken IDT/GDT/paging.

## U

**Userspace / user mode** — Code running at ring 3 with limited privileges. Not present in HelixOS v0.1; planned for v0.2.

## V

**VFS** — Virtual File System. A thin abstraction layer above concrete filesystem implementations. Allows mounting different filesystems under a unified path namespace.

**VGA** — Video Graphics Array. Specifically the text-mode subset: 80×25 cells at physical 0xB8000.

**VMM** — Virtual Memory Manager. Manages the virtual address space — installs page tables, registers the page-fault handler, hands out virtual regions.

## Z

**Zombie** — A task that has exited but whose PCB hasn't been freed. In v0.1, zombies stay in the task list forever; v0.2 will add a reaper.
