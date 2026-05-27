/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/isr.h
 *  Module:   arch/i386/isr
 *  Brief:    CPU exception handlers (vectors 0..31).
 *
 *  Exposes the shared registers_t structure used by both ISR and IRQ
 *  paths. The field order MUST match the push order in isr_stubs.asm
 *  and irq_stubs.asm exactly — change one, change all three.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_ISR_H
#define HELIX_ARCH_I386_ISR_H

#include <helix/types.h>

/**
 * Register snapshot pushed by every interrupt stub.
 *
 *  Order on stack (lowest address first, i.e. last pushed):
 *      ds                          ─ pushed by stub
 *      edi, esi, ebp, esp(dummy),  ─ pushed by `pusha`
 *      ebx, edx, ecx, eax
 *      int_no                      ─ pushed by stub
 *      err_code                    ─ pushed by CPU or stub (0 if none)
 *      eip, cs, eflags             ─ pushed by CPU
 *      useresp, ss                 ─ pushed by CPU when CPL change
 */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

/** Handler signature for both ISRs and IRQs. */
typedef void (*int_handler_t)(registers_t *regs);

/** Install ISR vectors 0..31 in the IDT. */
void isr_install(void);

/** Register a C handler for a specific exception vector (0..31). */
void isr_register(uint8_t vector, int_handler_t handler);

/**
 * Common C dispatcher called from isr_stubs.asm.
 * Looks up regs->int_no in the handler table and invokes it.
 */
void isr_dispatch(registers_t *regs);

#endif /* HELIX_ARCH_I386_ISR_H */
