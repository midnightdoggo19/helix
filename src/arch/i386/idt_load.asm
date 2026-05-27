; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/arch/i386/idt_load.asm
;  Module:   arch/i386/idt
;  Brief:    Load IDTR via the LIDT instruction.
; ─────────────────────────────────────────────────────────────────────

section .text
global idt_load

idt_load:
    mov   eax, [esp + 4]          ; argument: pointer to idt_ptr_t
    lidt  [eax]
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
