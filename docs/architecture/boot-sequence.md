# Boot Sequence

This document walks the entire path from "power on" to "shell prompt." If you read one architecture doc to understand HelixOS, read this one.

## Stage 0 — BIOS / firmware

When the machine powers on, the BIOS (or UEFI in legacy-CSM mode under QEMU) runs POST and looks for a bootable medium. It finds our CD-ROM image, reads its first sectors, and hands control to the El Torito boot record — which is GRUB's stage 1.

We don't write any of this. Everything before GRUB is firmware code that comes with the machine or with QEMU.

## Stage 1 — GRUB

GRUB reads `/boot/grub/grub.cfg` from the ISO:

```cfg
menuentry "HelixOS" {
    multiboot /boot/helix.bin
    boot
}
```

It loads `/boot/helix.bin` into memory at the address specified by the binary's ELF program headers (1 MiB per our linker script), validates the Multiboot1 header (magic `0x1BADB002`), populates a `multiboot_info_t` somewhere in low memory describing the system, and jumps to the entry point with:

| Register | Value at entry |
|---|---|
| `EAX` | `0x2BADB002` (handshake magic; tells us GRUB loaded us correctly) |
| `EBX` | physical address of the `multiboot_info_t` |
| Mode | 32-bit protected mode (CR0.PE = 1) |
| Paging | **disabled** (CR0.PG = 0) |
| Interrupts | **disabled** (EFLAGS.IF = 0) |
| GDT | GRUB's transient one (don't rely on it) |
| Stack | undefined — we must set one up |

That last point matters: every kernel tutorial that crashes mysteriously after entry probably forgot that GRUB does not provide a stack.

## Stage 2 — `boot.asm`

The entry point is `_start` in [`src/boot/boot.asm`](../../src/boot/boot.asm). Its job is minimal:

```nasm
_start:
    cli                          ; defensive — IF should already be 0
    mov   esp, stack_top         ; install our 16 KiB stack
    xor   ebp, ebp               ; null-terminate the frame chain

    push  ebx                    ; multiboot info pointer (arg 1)
    push  eax                    ; multiboot magic        (arg 0)

    call  kmain                  ; jump into C
```

The stack is a 16 KiB region in `.bss` (`stack_bottom .. stack_top`). The `xor ebp, ebp` ensures backtraces from a future debugger terminate cleanly.

### Multiboot header

The header lives at the very top of the binary in the `.multiboot` section so GRUB finds it within the first 8 KiB:

```nasm
section .multiboot
align 4
    dd 0x1BADB002        ; magic
    dd 0x00000003        ; flags: page-align modules + provide memory map
    dd -(0x1BADB002 + 0x00000003)  ; checksum
```

The linker script places `.multiboot` first, then `.text`, then `.rodata`, `.data`, `.bss` — see [`memory-model.md`](memory-model.md).

## Stage 3 — `kmain()`

[`src/kernel/kmain.c`](../../src/kernel/kmain.c) is the conductor. It calls every init function in a strict order. The full sequence:

```mermaid
sequenceDiagram
    autonumber
    participant boot as boot.asm
    participant km   as kmain.c
    participant arch as arch/i386
    participant mem  as memory/
    participant drv  as drivers/
    participant proc as process/
    participant sys  as syscalls/
    participant fs   as fs/
    participant sh   as shell/

    boot->>km: kmain(eax, ebx)
    km->>drv: serial_init()
    km->>drv: vga_init()
    km->>arch: gdt_init()
    km->>arch: idt_init()
    km->>arch: isr_install()  (vectors 0..31)
    km->>arch: irq_install()  (vectors 32..47)
    km->>arch: pic_init()     (remap to 0x20..0x2F)
    km->>mem:  pmm_init(multiboot_info)
    km->>mem:  paging_init()  → CR0.PG = 1
    km->>mem:  vmm_init()     (registers vector 14)
    km->>mem:  heap_init()    (1 MiB @ 0x00C00000)
    km->>arch: STI            (preemption now possible)
    km->>drv:  pit_init()     (100 Hz, IRQ 0 unmasked)
    km->>drv:  kbd_init()     (IRQ 1 unmasked)
    km->>proc: task_init()    (install init PCB)
    km->>sys:  syscall_init() (INT 0x80 gate, DPL=3)
    km->>fs:   ramfs_init()   (build /, /etc/motd, /README)
    km->>proc: task_create("shell", shell_main)
    km->>km:   idle: for(;;) cpu_halt()
    proc-->>sh: scheduler picks shell on next PIT IRQ
    sh->>sh:   shell_main() → MOTD → prompt
```

### Why this order

The dependency analysis behind each step:

| Step | Depends on |
|---|---|
| `serial`, `vga` | Nothing — pure port I/O |
| `gdt` | Nothing — sets up its own segments |
| `idt`, `isr_install`, `irq_install` | `gdt` (selectors used in gate descriptors) |
| `pic_init` | Must come **before** STI, or stray IRQs fire at wrong vectors |
| `pmm_init` | Multiboot info pointer (passed from `boot.asm`) |
| `paging_init` | `pmm` (needs to allocate page-table frames) |
| `vmm_init` | `isr_install` (registers vector 14 handler) |
| `heap_init` | `vmm_alloc_region` to map its backing |
| `STI` | All exception/IRQ infrastructure must be in place first |
| `pit_init` | `irq_install` (registers IRQ 0 handler), `pic_init` (unmask) |
| `task_init` | `heap` (PCB allocations), `pit` (so `task_sleep_ms` can work) |
| `syscall_init` | `idt` (installs vector 0x80) |
| `ramfs_init` | `heap` (file/directory nodes) |
| `task_create("shell", ...)` | Everything above |

### What gets logged

The boot console (VGA + serial) prints a uniform pattern:

```
[ok] GDT       initialized
[ok] IDT       initialized
[ok] ISRs      initialized
[ok] IRQs      initialized
[ok] PIC       initialized

[pmm] 32768 total frames, 306 used, 32462 free (129848 KiB usable)
[pmm] bitmap at 0x00111000 (4096 bytes)
[paging] 8 MiB identity-mapped, paging ON (CR3=0x0010f000)
[vmm ] page-fault handler installed (vector 14)
[heap] 1024 KiB heap mapped at 0x00c00000
[ok] interrupts initialized

[pit ] 100 Hz (divisor 11931), IRQ 0 unmasked
[kbd ] PS/2 keyboard ready (IRQ 1 unmasked)
[sched] round-robin scheduler online
[task] init task installed (pid 0)
[sys ] INT 0x80 gate installed, 9 syscalls registered
[ramfs] mounted at / with 4 entries seeded

[ READY ] HelixOS Phase 3f -- spawning shell

[task] created 'shell' (pid 1, ...)
Welcome to HelixOS -- 32-bit educational kernel
Type 'help' to list shell commands.

helix:/$
```

Boot time on QEMU: about **1.5 seconds** from GRUB hand-off to prompt.

## What could go wrong, and where

| Symptom | Likely cause | First diagnostic |
|---|---|---|
| QEMU triple-faults instantly | Bug in `boot.asm`, GDT loaded incorrectly | Check serial log — does `serial initialized` print? |
| Boots but no VGA output | VGA init crashed, but serial still works | Watch the serial log over `-serial stdio` |
| Hangs after "interrupts initialized" | `STI` triggered an IRQ before its handler was installed | Check PIC masks; `pic_init` should mask everything by default |
| "Page fault" panic immediately after `paging_init` | Some code touched an unmapped address | Check the CR2 dump on the panic banner |
| Boots but keystrokes do nothing | IRQ 1 unmasked but EOI not sent in time (see [ADR-005](../design/adr/0005-eoi-before-handler.md)) | Confirm `pic_send_eoi` runs before `irq_dispatch` invokes the handler |
| Workers run all 6 iterations in 3 ticks | Initial task stack frame has EFLAGS in the wrong slot — IF=0 after first context switch | Check `task_create` — EFLAGS must be at the lowest address of the saved frame |

## Glossary cross-reference

- **Multiboot1** — boot protocol spec used by GRUB. [Official spec](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html).
- **GDT** — see [interrupt-handling.md](interrupt-handling.md#gdt).
- **IDT** — see [interrupt-handling.md](interrupt-handling.md#idt).
- **CR0.PG / CR3** — see [memory-model.md](memory-model.md#enabling-paging).
- **PIT** — Programmable Interval Timer; see [scheduler-design.md](scheduler-design.md).
