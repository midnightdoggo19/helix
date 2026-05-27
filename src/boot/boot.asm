; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/boot/boot.asm
;  Module:   boot
;  Brief:    Multiboot1-compliant entry point. Called by GRUB.
;
;  At the moment GRUB transfers control to us:
;    * protected mode is active (CR0.PE = 1)
;    * paging is disabled
;    * interrupts are disabled
;    * EAX = multiboot magic (0x2BADB002)
;    * EBX = physical address of the multiboot info structure
;    * GDT is GRUB's transient one — we will replace it in arch/gdt.c
;
;  Our job here is minimal:
;    1. Install a fixed-size kernel stack.
;    2. Pass EAX/EBX as arguments to kmain().
;    3. If kmain ever returns, hang.
; ─────────────────────────────────────────────────────────────────────

; ── Multiboot1 header ────────────────────────────────────────────────
MB_MAGIC      equ 0x1BADB002
MB_FLAGS      equ 0x00000003           ; bit0 = page align, bit1 = mem map
MB_CHECKSUM   equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

; ── 16 KiB initial kernel stack ──────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

; ── Entry point ──────────────────────────────────────────────────────
section .text
global _start
extern kmain

_start:
    cli                                 ; interrupts off until IDT is set
    mov   esp, stack_top                ; install kernel stack
    xor   ebp, ebp                      ; terminate stack-frame chain

    push  ebx                           ; arg 2: multiboot info pointer
    push  eax                           ; arg 1: multiboot magic

    call  kmain                         ; jump into C-land

    ; kmain() is declared NORETURN, but defend against bugs.
.hang:
    cli
    hlt
    jmp .hang

; Mark the stack non-executable for ELF NX policy.
section .note.GNU-stack noalloc noexec nowrite progbits
