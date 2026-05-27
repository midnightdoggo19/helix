# Kernel API Reference

This document indexes the public functions of every kernel subsystem. It complements (not replaces) the Doxygen comments in the headers — go to the header for the full signature, parameter docs, and edge cases.

## Conventions

- All exported names use lowercase `snake_case`.
- Each subsystem prefixes its public functions (`pmm_*`, `vmm_*`, `task_*`, ...).
- Internal helpers are `static` and never exported.
- "Returns NULL" or "Returns false" indicates allocation failure — callers must check.

## `lib/` — freestanding libc subset

| Function | Header | Notes |
|---|---|---|
| `memcpy`, `memmove`, `memset`, `memcmp` | [`string.h`](../../src/lib/string.h) | Standard semantics |
| `strlen`, `strcmp`, `strncmp`, `strcpy`, `strncpy`, `strchr` | [`string.h`](../../src/lib/string.h) | Standard semantics |
| `kprintf(fmt, ...)` | [`printf.h`](../../src/lib/printf.h) | Supports `%c %s %d %i %u %x %X %p %%`; no floats |
| `kvprintf(sink, fmt, ap)` | [`printf.h`](../../src/lib/printf.h) | Generic — caller supplies the `putc` |
| `kputc(c)`, `kputs(s)` | [`printf.h`](../../src/lib/printf.h) | Routes to VGA + serial |

## `arch/i386/` — hardware abstraction

| Function | Header | Notes |
|---|---|---|
| `inb`, `inw`, `outb`, `outw`, `io_wait` | [`io.h`](../../src/arch/i386/io.h) | Inline; one x86 instruction each |
| `cpu_cli`, `cpu_sti`, `cpu_halt`, `cpu_hang`, `cpu_read_eflags` | [`cpu.h`](../../src/arch/i386/cpu.h) | Inline |
| `gdt_init` | [`gdt.h`](../../src/arch/i386/gdt.h) | Installs 5-entry flat GDT |
| `idt_init`, `idt_set_gate(vec, handler, sel, attr)` | [`idt.h`](../../src/arch/i386/idt.h) | |
| `isr_install`, `isr_register(vec, fn)` | [`isr.h`](../../src/arch/i386/isr.h) | vec ∈ [0, 31] |
| `irq_install`, `irq_register(irq, fn)`, `irq_unregister(irq)` | [`irq.h`](../../src/arch/i386/irq.h) | irq ∈ [0, 15] |
| `pic_init`, `pic_send_eoi(irq)`, `pic_mask(irq)`, `pic_unmask(irq)` | [`pic.h`](../../src/arch/i386/pic.h) | |

## `memory/` — memory management

| Function | Header | Notes |
|---|---|---|
| `pmm_init(mb)` | [`pmm.h`](../../src/memory/pmm.h) | Parses Multiboot mmap |
| `pmm_alloc_frame()` | [`pmm.h`](../../src/memory/pmm.h) | Returns phys addr or `PMM_NO_FRAME` |
| `pmm_alloc_frames(n)` | [`pmm.h`](../../src/memory/pmm.h) | n contiguous frames |
| `pmm_free_frame(phys)` | [`pmm.h`](../../src/memory/pmm.h) | |
| `pmm_total_frames`, `pmm_used_frames`, `pmm_free_frames` | [`pmm.h`](../../src/memory/pmm.h) | Statistics |
| `paging_init()` | [`paging.h`](../../src/memory/paging.h) | Enables CR0.PG |
| `paging_map(virt, phys, flags)`, `paging_unmap(virt)`, `paging_translate(virt)`, `paging_flush_tlb()` | [`paging.h`](../../src/memory/paging.h) | flags: `PAGE_RW`, `PAGE_USER`, etc. |
| `vmm_init()` | [`vmm.h`](../../src/memory/vmm.h) | Registers page-fault handler |
| `vmm_alloc_region(virt, pages, flags)`, `vmm_free_region(virt, pages)` | [`vmm.h`](../../src/memory/vmm.h) | Atomic — rolls back on partial failure |
| `heap_init()` | [`heap.h`](../../src/memory/heap.h) | Maps 1 MiB region |
| `kmalloc(size)`, `kfree(ptr)`, `kcalloc(n, size)` | [`heap.h`](../../src/memory/heap.h) | Standard `malloc`/`free` semantics |
| `heap_used`, `heap_total` | [`heap.h`](../../src/memory/heap.h) | Statistics |

## `drivers/` — device drivers

| Function | Header | Notes |
|---|---|---|
| `serial_init`, `serial_putc(c)`, `serial_puts(s)` | [`serial.h`](../../src/drivers/serial/serial.h) | COM1 only |
| `vga_init`, `vga_clear`, `vga_putc(c)`, `vga_puts(s)` | [`vga.h`](../../src/drivers/vga/vga.h) | Handles `\n \r \b \t` |
| `vga_set_color(fg, bg)`, `vga_set_cursor(x, y)` | [`vga.h`](../../src/drivers/vga/vga.h) | 16 colors each |
| `pit_init`, `pit_ticks`, `pit_uptime_ms`, `pit_sleep_ms(ms)` | [`pit.h`](../../src/drivers/timer/pit.h) | 100 Hz fixed |
| `kbd_init`, `kbd_getchar` (blocking), `kbd_poll(&c)`, `kbd_has_input` | [`keyboard.h`](../../src/drivers/keyboard/keyboard.h) | US-QWERTY |

## `process/` — tasks and scheduling

| Function | Header | Notes |
|---|---|---|
| `task_init()` | [`task.h`](../../src/process/task.h) | Installs init task (pid 0) |
| `task_create(name, entry)` | [`task.h`](../../src/process/task.h) | Returns new PCB or NULL |
| `task_current()`, `task_set_current(t)` | [`task.h`](../../src/process/task.h) | |
| `task_yield()`, `task_sleep_ms(ms)`, `task_exit()` (noreturn) | [`task.h`](../../src/process/task.h) | |
| `task_for_each(fn, ctx)` | [`task.h`](../../src/process/task.h) | Visitor pattern; used by `SYS_PS` |
| `scheduler_init(t)`, `schedule()`, `scheduler_tick()` | [`scheduler.h`](../../src/process/scheduler.h) | |
| `context_switch(&old_esp, new_esp)` | [`scheduler.h`](../../src/process/scheduler.h) | Defined in `context_switch.asm` |
| `spin_lock`, `spin_unlock` | [`sync.h`](../../src/process/sync.h) | Save+cli |
| `mutex_lock`, `mutex_unlock` | [`sync.h`](../../src/process/sync.h) | Yielding |
| `sem_init`, `sem_wait`, `sem_signal` | [`sync.h`](../../src/process/sync.h) | Counting |

## `fs/` — virtual filesystem

| Function | Header | Notes |
|---|---|---|
| `ramfs_init()` | [`ramfs.h`](../../src/fs/ramfs.h) | Builds root + seeds initial tree |
| `vfs_root()` | [`vfs.h`](../../src/fs/vfs.h) | |
| `vfs_lookup(path)` | [`vfs.h`](../../src/fs/vfs.h) | Returns node or NULL |
| `vfs_read(node, offset, buf, count)` | [`vfs.h`](../../src/fs/vfs.h) | |
| `vfs_write_append(node, buf, count)` | [`vfs.h`](../../src/fs/vfs.h) | |
| `vfs_touch(path)`, `vfs_mkdir(path)`, `vfs_remove(path)` | [`vfs.h`](../../src/fs/vfs.h) | |

## `syscalls/` — system call layer

| Function | Header | Notes |
|---|---|---|
| `syscall_init()` | [`syscall.h`](../../src/syscalls/syscall.h) | Installs INT 0x80 gate |
| `syscall_dispatch(regs)` | (internal) | Called from assembly |
| `syscall0`, `syscall1`, `syscall3` | [`syscall.h`](../../src/syscalls/syscall.h) | Inline wrappers |

See [`syscalls.md`](syscalls.md) for the call-by-call ABI.

## `shell/` — interactive shell

| Function | Header | Notes |
|---|---|---|
| `shell_main()` | [`shell.h`](../../src/shell/shell.h) | Task entry; never returns |
| `cmd_dispatch(argc, argv)` | (internal in `commands.c`) | Used by the shell main loop |

## `kernel/` — top-level

| Function | Header | Notes |
|---|---|---|
| `kmain(magic, mb)` | (called from boot.asm) | Entry from assembly |
| `kpanic(fmt, ...)` (noreturn) | [`<helix/kernel.h>`](../../include/helix/kernel.h) | Red banner, halt |
| `KASSERT(cond)` | [`<helix/kernel.h>`](../../include/helix/kernel.h) | Panics if cond false |
| `helix_banner()` | [`<helix/version.h>`](../../include/helix/version.h) | "HelixOS 0.1.0 ..." string |

## Header-only utilities

`<helix/kernel.h>` macros:

| Macro | Purpose |
|---|---|
| `ARRAY_SIZE(a)` | `sizeof(a) / sizeof(a[0])` |
| `ALIGN_UP(x, a)`, `ALIGN_DOWN(x, a)`, `IS_ALIGNED(x, a)` | Power-of-two alignment |
| `MIN(a,b)`, `MAX(a,b)` | Triple-eval safe |
| `UNUSED(x)` | Silence -Wunused-parameter |
| `PACKED`, `ALIGNED(n)`, `NORETURN`, `USED`, `INLINE`, `COLD` | GCC attribute shorthands |
