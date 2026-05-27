/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/pic.c
 *  Module:   arch/i386/pic
 *  Brief:    8259 PIC remap + masking.
 *
 *  Initialization is done by the canonical ICW1..ICW4 sequence:
 *      ICW1 - 0x11 = "I will send 4 ICWs, edge-triggered, cascaded"
 *      ICW2 - vector base for this PIC
 *      ICW3 - master: bitmap of slave wiring (slave on IRQ2 -> 0x04)
 *             slave : its slave-ID number (2)
 *      ICW4 - 0x01 = 8086/8088 mode, normal EOI
 * ─────────────────────────────────────────────────────────────────────
 */
#include "pic.h"
#include "io.h"

#define ICW1_INIT           0x10
#define ICW1_ICW4           0x01
#define ICW4_8086           0x01

void pic_init(void)
{
    /* Save current masks (we'll restore after remap). */
    uint8_t mask_master = inb(PIC_MASTER_DATA);
    uint8_t mask_slave  = inb(PIC_SLAVE_DATA);

    /* ICW1: start initialization (cascade + ICW4 needed) */
    outb(PIC_MASTER_CMD, ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC_SLAVE_CMD,  ICW1_INIT | ICW1_ICW4); io_wait();

    /* ICW2: vector offsets */
    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET); io_wait();
    outb(PIC_SLAVE_DATA,  PIC_SLAVE_OFFSET);  io_wait();

    /* ICW3: master/slave wiring (slave is connected to master's IRQ 2) */
    outb(PIC_MASTER_DATA, 0x04); io_wait();   /* bit 2 set */
    outb(PIC_SLAVE_DATA,  0x02); io_wait();   /* slave ID = 2 */

    /* ICW4: 8086 mode */
    outb(PIC_MASTER_DATA, ICW4_8086); io_wait();
    outb(PIC_SLAVE_DATA,  ICW4_8086); io_wait();

    /* Mask everything by default — drivers unmask their own lines. */
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA,  0xFF);

    /* Touch the saved masks so -Wunused-variable doesn't complain when
     * a future revision decides not to restore them. */
    (void)mask_master;
    (void)mask_slave;
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC_SLAVE_CMD, PIC_EOI);
    outb(PIC_MASTER_CMD, PIC_EOI);
}

void pic_unmask(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }
    value = inb(port) & (uint8_t)~(1U << irq);
    outb(port, value);
}

void pic_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }
    value = inb(port) | (uint8_t)(1U << irq);
    outb(port, value);
}
