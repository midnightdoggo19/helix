; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/arch/i386/irq_stubs.asm
;  Module:   arch/i386/irq
;  Brief:    16 IRQ trampolines (vectors 32..47). Structurally
;            identical to ISRs but always push 0 as the error code.
; ─────────────────────────────────────────────────────────────────────

extern irq_dispatch

%macro IRQ_STUB 2  ; %1 = IRQ line (0..15), %2 = vector (32..47)
global irq%1
irq%1:
    cli
    push  dword 0                 ; dummy error code
    push  dword %2                ; interrupt number
    jmp   irq_common
%endmacro

section .text
irq_common:
    pusha

    xor   eax, eax
    mov   ax, ds
    push  eax

    mov   ax, 0x10
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax

    push  esp
    call  irq_dispatch
    add   esp, 4

    pop   eax
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax

    popa
    add   esp, 8
    sti
    iret

IRQ_STUB 0,  32
IRQ_STUB 1,  33
IRQ_STUB 2,  34
IRQ_STUB 3,  35
IRQ_STUB 4,  36
IRQ_STUB 5,  37
IRQ_STUB 6,  38
IRQ_STUB 7,  39
IRQ_STUB 8,  40
IRQ_STUB 9,  41
IRQ_STUB 10, 42
IRQ_STUB 11, 43
IRQ_STUB 12, 44
IRQ_STUB 13, 45
IRQ_STUB 14, 46
IRQ_STUB 15, 47

section .note.GNU-stack noalloc noexec nowrite progbits
