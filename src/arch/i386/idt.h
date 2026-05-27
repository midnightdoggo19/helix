/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/idt.h
 *  Module:   arch/i386/idt
 *  Brief:    Interrupt Descriptor Table.
 *
 *  256 gates total. Vectors 0..31 are CPU exceptions, 32..47 are the
 *  remapped hardware IRQs, 0x80 is the system call gate.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_IDT_H
#define HELIX_ARCH_I386_IDT_H

#include <helix/kernel.h>

#define IDT_NUM_ENTRIES   256

/* Gate type/attribute bytes */
#define IDT_GATE_PRESENT  0x80
#define IDT_GATE_DPL0     0x00
#define IDT_GATE_DPL3     0x60
#define IDT_GATE_INT32    0x0E    /* 32-bit interrupt gate */

/** One 8-byte IDT gate. */
typedef struct PACKED idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} idt_entry_t;

/** Operand of the LIDT instruction. */
typedef struct PACKED idt_ptr {
    uint16_t limit;
    uint32_t base;
} idt_ptr_t;

/** Build a fully zeroed IDT and load it. ISRs and IRQs install gates later. */
void idt_init(void);

/** Install a single gate. */
void idt_set_gate(uint8_t vector, uint32_t handler,
                  uint16_t selector, uint8_t type_attr);

#endif /* HELIX_ARCH_I386_IDT_H */
