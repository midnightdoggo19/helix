; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/syscalls/syscall_dispatch.asm
;  Module:   syscalls
;  Brief:    INT 0x80 entry. Saves caller registers, calls C dispatcher,
;            returns result in EAX.
;
;  At entry, the user (or kernel-mode caller) provided:
;      eax = syscall number
;      ebx, ecx, edx, esi, edi = args
;
;  We build a registers_t frame compatible with ISR/IRQ stubs so the
;  C dispatcher can introspect the caller if it wants (useful later
;  for `sys_ps` to identify the calling task by EIP).
; ─────────────────────────────────────────────────────────────────────

extern syscall_dispatch

section .text
global syscall_entry

syscall_entry:
    cli
    push  dword 0                 ; dummy error code
    push  dword 0x80              ; "interrupt number"

    pusha                         ; saves eax,ecx,edx,ebx,esp,ebp,esi,edi

    xor   eax, eax
    mov   ax, ds
    push  eax                     ; save data segment

    mov   ax, 0x10                ; load kernel data segs
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax

    push  esp                     ; arg: pointer to registers_t
    call  syscall_dispatch
    add   esp, 4                  ; discard arg

    ; Return value was written into the saved EAX slot by the C side.
    pop   eax                     ; restore ds
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax

    popa
    add   esp, 8                  ; discard int_no + err_code
    sti
    iret

section .note.GNU-stack noalloc noexec nowrite progbits
