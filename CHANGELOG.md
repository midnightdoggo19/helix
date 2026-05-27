# Changelog

All notable changes to HelixOS are documented here. Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); the project uses [Semantic Versioning](https://semver.org).

## [Unreleased]

Nothing yet.

## [0.1.0] — 2026-05-26 — "Genesis"

The initial release. Everything described in the architecture docs.

### Added

- **Boot**: Multiboot1 entry via GRUB. Kernel loads at 1 MiB, sets up 16 KiB stack, jumps to `kmain`.
- **HAL** (`src/arch/i386/`): 5-entry GDT (kernel + user code/data), 256-entry IDT, ISR stubs for vectors 0..31, IRQ stubs for vectors 32..47, 8259 PIC remapped to 0x20..0x2F, port I/O and CPU intrinsic wrappers.
- **Memory**:
    - PMM with 4 KiB-bitmap allocator; tracks per-frame usage.
    - 32-bit paging with 4 KiB pages; identity-maps first 8 MiB.
    - Page-fault handler decoding CR2 + error code into a readable panic.
    - 1 MiB kernel heap at 0x00C00000 with first-fit + split + coalesce.
- **Drivers**:
    - VGA 80×25 text mode with color, scroll, hardware cursor.
    - Serial COM1 at 38400 8N1 for debug output.
    - PIT at 100 Hz driving the scheduler.
    - PS/2 keyboard with US-QWERTY scancode set, shift + caps-lock tracking, 128-byte ring buffer.
- **Process subsystem**:
    - Task control block (`task_t`, 40 bytes), 8 KiB per-task kernel stacks.
    - Round-Robin preemptive scheduler.
    - Context switch in NASM, saving callee-saved registers + EFLAGS.
    - Sleep with millisecond resolution (tick granularity 10 ms).
    - Spinlock, mutex, semaphore primitives.
- **Syscalls**: `INT 0x80` gate at DPL=3. Nine syscalls: `EXIT`, `WRITE`, `READ`, `GETPID`, `YIELD`, `SLEEP_MS`, `UPTIME`, `PS`, `MEMINFO`.
- **Filesystem**: VFS layer + RAMFS mounted at `/`. Hierarchical directories, growable files. Seeded with `/etc/motd` and `/README` at boot.
- **Shell**: Interactive task with 14 built-in commands — `help`, `version`, `clear`, `echo`, `uptime`, `ps`, `mem`, `ls`, `cat`, `touch`, `mkdir`, `rm`, `write`, `reboot`. Line editor with backspace.
- **Build**: Cross-compile via `i686-elf-gcc`; one-shot `make iso` produces a GRUB-bootable ISO; `make run` boots under QEMU.
- **Docs**: README, architecture overview, boot sequence, memory model, interrupt handling, scheduler design; full API reference; setup, developer, debugging, testing, performance, modules, FAQ, glossary; six ADRs (language choice, monolithic kernel, Multiboot1, bitmap PMM, EOI-before-handler, context-switch frame layout); five Mermaid diagrams.

### Fixed (during 0.1 development)

- IRQ dispatcher now sends EOI **before** invoking handlers, so the PIT context switch doesn't leave IRQ 0 in service and block the keyboard. ([ADR-0005](docs/design/adr/0005-eoi-before-handler.md))
- Initial task stack frame now places EFLAGS at the lowest address so `popfd` reads it first, restoring IF=1 on every newly-switched-to task. ([ADR-0006](docs/design/adr/0006-context-switch-frame-layout.md))
- Eliminated shadowed `static task_t *current` definitions in `task.c` and `scheduler.c`; current task pointer now routed through `task_current()` / `task_set_current()` accessors.

### Notable measurements

- Kernel `.text`: 29.4 KiB
- Kernel `.bss`: 24.0 KiB
- Source: ~4,940 lines (C + ASM)
- Documentation: ~3,600 lines

[Unreleased]: https://github.com/prakhardewangan2005-hash/helix-os/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/prakhardewangan2005-hash/helix-os/releases/tag/v0.1.0
