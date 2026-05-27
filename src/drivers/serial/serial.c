/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/serial/serial.c
 *  Module:   drivers/serial
 *  Brief:    16550 UART driver implementation.
 *
 *  Register map (offset from COM base):
 *      +0  Data  / Divisor LSB    (RW)
 *      +1  IER   / Divisor MSB    (RW)
 *      +2  IIR / FCR              (R / W)
 *      +3  LCR                    (RW)
 *      +4  MCR                    (RW)
 *      +5  LSR                    (R)
 *      +6  MSR                    (R)
 * ─────────────────────────────────────────────────────────────────────
 */
#include "serial.h"
#include "../../arch/i386/io.h"

#define COM         SERIAL_COM1_BASE

#define REG_DATA    (COM + 0)
#define REG_IER     (COM + 1)
#define REG_FCR     (COM + 2)
#define REG_LCR     (COM + 3)
#define REG_MCR     (COM + 4)
#define REG_LSR     (COM + 5)

#define LSR_THR_EMPTY  (1U << 5)   /* Transmitter Holding Register empty */

void serial_init(void)
{
    outb(REG_IER, 0x00);        /* disable all interrupts                  */
    outb(REG_LCR, 0x80);        /* enable DLAB to set divisor              */
    outb(REG_DATA, 0x03);       /* divisor low  = 3  → 38400 baud          */
    outb(REG_IER,  0x00);       /* divisor high = 0                        */
    outb(REG_LCR,  0x03);       /* 8 bits, no parity, 1 stop, DLAB off     */
    outb(REG_FCR,  0xC7);       /* enable FIFO, clear them, 14-byte thresh */
    outb(REG_MCR,  0x0B);       /* IRQs enabled, RTS/DSR set               */
}

static inline bool tx_ready(void)
{
    return (inb(REG_LSR) & LSR_THR_EMPTY) != 0;
}

void serial_putc(char c)
{
    /* Treat LF as CRLF for terminals that don't auto-translate. */
    if (c == '\n') {
        while (!tx_ready()) { /* spin */ }
        outb(REG_DATA, '\r');
    }
    while (!tx_ready()) { /* spin */ }
    outb(REG_DATA, (uint8_t)c);
}

void serial_puts(const char *s)
{
    while (*s)
        serial_putc(*s++);
}
