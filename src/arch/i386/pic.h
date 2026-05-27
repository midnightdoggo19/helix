/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/pic.h
 *  Module:   arch/i386/pic
 *  Brief:    8259 Programmable Interrupt Controller driver.
 *
 *  The legacy x86 has two cascaded 8259 PICs (master + slave) wiring
 *  16 hardware IRQ lines into a single CPU INTR pin. By default they
 *  raise vectors 0x08..0x0F (master) and 0x70..0x77 (slave), which
 *  collide with CPU exceptions. We remap to 0x20..0x2F so that:
 *
 *      IRQ 0  (PIT)        -> INT 0x20 (32)
 *      IRQ 1  (keyboard)   -> INT 0x21 (33)
 *      ...
 *      IRQ 15 (secondary)  -> INT 0x2F (47)
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_PIC_H
#define HELIX_ARCH_I386_PIC_H

#include <helix/types.h>

#define PIC_MASTER_OFFSET   0x20   /* IRQ 0..7  -> INT 32..39 */
#define PIC_SLAVE_OFFSET    0x28   /* IRQ 8..15 -> INT 40..47 */

#define PIC_MASTER_CMD      0x20
#define PIC_MASTER_DATA     0x21
#define PIC_SLAVE_CMD       0xA0
#define PIC_SLAVE_DATA      0xA1

#define PIC_EOI             0x20   /* End-Of-Interrupt command */

/** Remap and initialize both PICs, all lines masked. */
void pic_init(void);

/** Acknowledge an IRQ. Must be called from every IRQ handler. */
void pic_send_eoi(uint8_t irq);

/** Unmask a single IRQ line (0..15). */
void pic_unmask(uint8_t irq);

/** Mask a single IRQ line (0..15). */
void pic_mask(uint8_t irq);

#endif /* HELIX_ARCH_I386_PIC_H */
