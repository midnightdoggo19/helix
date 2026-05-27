# ADR-0003: Boot via GRUB (Multiboot1) instead of a custom bootloader

**Status**: Accepted
**Date**: 2026-05-25

## Context

The kernel needs *something* between BIOS power-on and `kmain()`. Options:

1. **Custom multi-stage bootloader** — write our own MBR stub that loads further sectors, switches to protected mode, sets up a transient GDT, and jumps to the kernel
2. **GRUB via Multiboot1** — GRUB does all of the above; we only write the Multiboot header
3. **GRUB via Multiboot2** — newer protocol, mostly relevant for UEFI / 64-bit
4. **U-Boot** — embedded-focused; overkill for x86

## Decision

**GRUB with the Multiboot1 protocol.**

## Why GRUB

A custom bootloader is a separate ~800-LoC project. It teaches you:

- 16-bit real mode → 32-bit protected mode transition
- A20 line activation
- BIOS disk I/O (INT 0x13)
- ELF parsing in 16-bit assembly

All of which are interesting *once*, after which you have a worse bootloader than GRUB. The reviewer of a portfolio kernel doesn't care that you re-implemented GRUB — they care that the *kernel* is well-engineered.

GRUB gives us:

- Loaded at 1 MiB, in 32-bit protected mode, with a stack-less but otherwise sane CPU state
- A `multiboot_info_t` describing memory layout, command-line args, boot device, and modules
- A drop-in ISO build via `grub-mkrescue`
- Works on real hardware, in QEMU, in VirtualBox, in VMware

## Why Multiboot1 and not Multiboot2

Multiboot1 is enough. The Multiboot2 spec adds:

- Tagged-list info structure (more extensible, more complex to parse)
- EFI framebuffer info
- Better support for VBE modes

We don't need any of that. The 32-byte Multiboot1 header is trivial to embed, the info struct is a flat C struct, and every tutorial uses Multiboot1. If we ever add a framebuffer driver we can revisit.

## Custom bootloader as a directory

The repository structure reserves `src/boot/` — it's where a custom bootloader *would* go if we ever wrote one. For now, the only files in there are the Multiboot header (`boot.asm`), the multiboot info parser, and the linker script. If a v0.5 decides to replace GRUB with a custom loader, the layout doesn't change.

## Consequences

**Positive**

- One-shot build via `grub-mkrescue`; the ISO works everywhere
- We can focus 100% of our boot-stage code on the Multiboot header
- The memory map handed to us is more accurate than what we could probe ourselves
- Multiple kernels (debug build, demo build) can share one ISO via different menuentries

**Negative**

- We depend on GRUB being installed in the build environment
- The boot menu adds a half-second to QEMU runs (mitigated by `set timeout=0`)

**Build dependency**: `grub-pc-bin`, `grub-common`, `xorriso`, `mtools`. The Dockerfile and the `build-toolchain.sh` script install these.

## Future direction

A "minimal kernel reaches userspace" tutorial-style project could legitimately replace GRUB with a 1-floppy-sector bootloader. We don't, because we have higher-leverage features to spend complexity on.
