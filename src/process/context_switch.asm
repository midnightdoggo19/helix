; SPDX-License-Identifier: MIT
; Copyright (c) 2026 HelixOS Authors
;
; ─────────────────────────────────────────────────────────────────────
;  File:     src/process/context_switch.asm
;  Module:   process/scheduler
;  Brief:    void context_switch(uint32_t *old_esp_ptr, uint32_t new_esp)
;            and the per-task entry trampoline.
;
;  We use a call-based switch (not iret-based). Only callee-saved
;  registers are preserved (EBX, ESI, EDI, EBP), plus EFLAGS so the
;  interrupt-enable flag is per-task. EAX/ECX/EDX are caller-saved by
;  the System V i386 ABI — any function calling context_switch from C
;  already expects them to be clobbered.
;
;  Stack frame layout that this routine creates/consumes:
;
;       low addr
;       +------------------+
;       |  EDI             |  <-- saved esp points here
;       |  ESI             |
;       |  EBX             |
;       |  EBP             |
;       |  EFLAGS          |
;       |  return address  |  (back into caller of context_switch)
;       +------------------+
;       high addr
; ─────────────────────────────────────────────────────────────────────

section .text
global context_switch
global task_trampoline

context_switch:
    push  ebp
    push  ebx
    push  esi
    push  edi
    pushfd                        ; save EFLAGS last so popfd is first

    ; Arguments (offsets after our 5 pushes + the 4-byte return addr = 24):
    ;   [esp+24] -> old_esp_ptr
    ;   [esp+28] -> new_esp
    mov   eax, [esp + 24]
    mov   [eax], esp              ; *old_esp_ptr = current ESP

    mov   esp, [esp + 28]         ; ESP <- new_esp

    popfd                         ; restore EFLAGS (incl. IF) for the new task
    pop   edi
    pop   esi
    pop   ebx
    pop   ebp
    ret                           ; pops next return target from new task's stack

; ─────────────────────────────────────────────────────────────────────
;  task_trampoline — first thing every newly created task runs.
;
;  Entry stack (after context_switch did its pops + ret to here):
;       [esp+0]  entry function address  (call target)
;       [esp+4]  task_exit               (fallback if entry returns)
;
;  EFLAGS already has IF=1 from the popfd in context_switch, so
;  interrupts are on.
; ─────────────────────────────────────────────────────────────────────
task_trampoline:
    sti                           ; defense in depth; popfd already enabled IRQs
    ret                           ; pops entry, jumps into it

section .note.GNU-stack noalloc noexec nowrite progbits
