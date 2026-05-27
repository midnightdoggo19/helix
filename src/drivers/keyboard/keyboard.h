/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/keyboard/keyboard.h
 *  Module:   drivers/keyboard
 *  Brief:    PS/2 keyboard driver (IRQ 1, ring buffer, US layout).
 *
 *  The IRQ 1 handler is the *producer*: it reads a scancode from
 *  port 0x60, converts it to ASCII through scancodes.h, and writes
 *  it into a ring buffer.
 *
 *  Consumers call kbd_getchar() (blocking) or kbd_poll() (non-block).
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_DRIVERS_KEYBOARD_H
#define HELIX_DRIVERS_KEYBOARD_H

#include <helix/types.h>

void kbd_init(void);

/** Block until a character is available, then return it. */
char kbd_getchar(void);

/**
 * Non-blocking peek. Returns 1 and writes *c if a key is buffered,
 * 0 otherwise.
 */
int  kbd_poll(char *c);

/** True if any character is currently buffered. */
bool kbd_has_input(void);

#endif /* HELIX_DRIVERS_KEYBOARD_H */
