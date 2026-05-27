/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/serial/serial.h
 *  Module:   drivers/serial
 *  Brief:    16550 UART driver for the COM1 port (debug logging).
 *
 *  Running QEMU with `-serial stdio` redirects COM1 to the host
 *  terminal, giving us a kernel log channel that works before VGA
 *  is initialized — and that survives a screen full of garbage.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_DRIVERS_SERIAL_H
#define HELIX_DRIVERS_SERIAL_H

#include <helix/types.h>

#define SERIAL_COM1_BASE   0x3F8

/** Initialize COM1 to 38400 baud, 8N1, FIFOs enabled. */
void serial_init(void);

/** Write one character to COM1, busy-waiting for the transmit buffer. */
void serial_putc(char c);

/** Write a null-terminated string to COM1. */
void serial_puts(const char *s);

#endif /* HELIX_DRIVERS_SERIAL_H */
