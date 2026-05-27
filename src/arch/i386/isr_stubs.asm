; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/arch/i386/isr_stubs.asm
;  Module:   arch/i386/isr
;  Brief:    32 trampolines (one per CPU exception) that build the
;            shared registers_t frame and jump to isr_common.
;
;  Some exceptions push an error code automatically (8, 10..14, 17, 21);
;  the others don't. To keep the frame uniform we push a dummy 0
;  error code from the assembly stub for the latter group.
; ─────────────────────────────────────────────────────────────────────

extern isr_dispatch

; ── Macros ───────────────────────────────────────────────────────────

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push  dword 0                 ; dummy error code
    push  dword %1                ; interrupt number
    jmp   isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
                                  ; CPU already pushed error code
    push  dword %1                ; interrupt number
    jmp   isr_common
%endmacro

; ── Common dispatch path ─────────────────────────────────────────────
;
;  Stack state at entry to isr_common (from low addr → high addr):
;       int_no, err_code, EIP, CS, EFLAGS [, USERESP, SS if CPL changed]
;  After this routine builds the frame:
;       ds, edi, esi, ebp, esp, ebx, edx, ecx, eax, int_no, err_code, ...
;  which exactly matches registers_t.

section .text
isr_common:
    pusha                         ; push eax,ecx,edx,ebx,esp,ebp,esi,edi

    xor   eax, eax
    mov   ax, ds
    push  eax                     ; save data segment

    mov   ax, 0x10                ; load kernel data
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax

    push  esp                     ; argument: pointer to registers_t
    call  isr_dispatch
    add   esp, 4                  ; discard argument

    pop   eax                     ; restore data segment
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax

    popa
    add   esp, 8                  ; discard int_no + err_code
    sti
    iret

; ── 32 exception stubs ───────────────────────────────────────────────
ISR_NOERR 0     ; Divide-by-zero
ISR_NOERR 1     ; Debug
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; Breakpoint
ISR_NOERR 4     ; Overflow
ISR_NOERR 5     ; BOUND range exceeded
ISR_NOERR 6     ; Invalid opcode
ISR_NOERR 7     ; Device not available
ISR_ERR   8     ; Double fault
ISR_NOERR 9     ; (legacy) coprocessor segment overrun
ISR_ERR   10    ; Invalid TSS
ISR_ERR   11    ; Segment not present
ISR_ERR   12    ; Stack-segment fault
ISR_ERR   13    ; General protection fault
ISR_ERR   14    ; Page fault
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; x87 FP exception
ISR_ERR   17    ; Alignment check
ISR_NOERR 18    ; Machine check
ISR_NOERR 19    ; SIMD FP exception
ISR_NOERR 20    ; Virtualization
ISR_ERR   21    ; Control-protection
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

section .note.GNU-stack noalloc noexec nowrite progbits
