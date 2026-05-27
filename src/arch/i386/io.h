/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/io.h
 *  Module:   arch/i386/io
 *  Brief:    Port-mapped I/O primitives (inb/outb and friends).
 *
 *  x86 talks to legacy hardware (PIC, PIT, keyboard, serial) through
 *  a separate I/O address space accessed with the IN/OUT instructions.
 *  Every device driver in HelixOS routes through these inlines.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_IO_H
#define HELIX_ARCH_I386_IO_H

#include <helix/types.h>

/** Write one byte to the given I/O port. */
static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/** Read one byte from the given I/O port. */
static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/** Write one word (16-bit) to the given I/O port. */
static inline void outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/** Read one word (16-bit) from the given I/O port. */
static inline uint16_t inw(uint16_t port)
{
    uint16_t val;
    __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/**
 * Short delay by writing to an unused port. Some legacy hardware
 * (notably the 8259 PIC) needs a few cycles between register writes.
 */
static inline void io_wait(void)
{
    __asm__ volatile("outb %%al, $0x80" : : "a"((uint8_t)0));
}

#endif /* HELIX_ARCH_I386_IO_H */
