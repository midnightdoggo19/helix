/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/irq.h
 *  Module:   arch/i386/irq
 *  Brief:    Hardware IRQ routing (vectors 32..47, after PIC remap).
 *
 *  Each device driver calls irq_register() with a callback. When the
 *  PIC raises an IRQ the common dispatcher invokes the registered
 *  callback (if any) and then acknowledges the PIC.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_IRQ_H
#define HELIX_ARCH_I386_IRQ_H

#include "isr.h"

/** Install gates for IRQs 0..15 (vectors 32..47). */
void irq_install(void);

/** Register a C handler for an IRQ line (0..15). */
void irq_register(uint8_t irq, int_handler_t handler);

/** Remove a handler. */
void irq_unregister(uint8_t irq);

/** Dispatcher invoked from irq_stubs.asm. */
void irq_dispatch(registers_t *regs);

#endif /* HELIX_ARCH_I386_IRQ_H */
