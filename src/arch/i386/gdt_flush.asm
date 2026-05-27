; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/arch/i386/gdt_flush.asm
;  Module:   arch/i386/gdt
;  Brief:    Load GDTR and reload segment registers.
;
;  Loading the GDT is not enough — the CPU caches the old segment
;  descriptors in shadow registers. We must reload every segment
;  register to make the new entries take effect. CS in particular
;  can only be reloaded by a far jump.
; ─────────────────────────────────────────────────────────────────────

section .text
global gdt_flush

gdt_flush:
    mov   eax, [esp + 4]          ; argument: pointer to gdt_ptr_t
    lgdt  [eax]                   ; load GDT register

    mov   ax, 0x10                ; 0x10 = kernel data selector
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    jmp   0x08:.reload_cs         ; 0x08 = kernel code; far jump reloads CS
.reload_cs:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
