/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/keyboard/scancodes.h
 *  Module:   drivers/keyboard
 *  Brief:    PS/2 scancode set 1 → ASCII lookup tables (US layout).
 *
 *  Index = scancode (make code only, 0x00..0x58).
 *  Value = ASCII character, or 0 if the key has no printable mapping
 *          (modifier keys, function keys, arrows, etc.).
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_DRIVERS_KEYBOARD_SCANCODES_H
#define HELIX_DRIVERS_KEYBOARD_SCANCODES_H

#include <helix/types.h>

/* Make-code values for tracked modifier keys */
#define SC_LCTRL    0x1D
#define SC_LSHIFT   0x2A
#define SC_RSHIFT   0x36
#define SC_LALT     0x38
#define SC_CAPSLOCK 0x3A

/* Press = 0x00..0x7F, release = previous | 0x80 */
#define SC_RELEASE_MASK 0x80

/* Index 0..0x39 covers all printable US-QWERTY keys in set 1. */
static const char scancode_to_ascii[60] = {
    /* 0x00 */ 0,    27,  '1', '2', '3', '4', '5', '6',
    /* 0x08 */ '7',  '8', '9', '0', '-', '=', '\b','\t',
    /* 0x10 */ 'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',
    /* 0x18 */ 'o',  'p', '[', ']', '\n', 0,  'a', 's',
    /* 0x20 */ 'd',  'f', 'g', 'h', 'j', 'k', 'l', ';',
    /* 0x28 */ '\'', '`', 0,  '\\','z', 'x', 'c', 'v',
    /* 0x30 */ 'b',  'n', 'm', ',', '.', '/', 0,  '*',
    /* 0x38 */ 0,    ' ', 0,   0,
};

static const char scancode_to_ascii_shift[60] = {
    /* 0x00 */ 0,    27,  '!', '@', '#', '$', '%', '^',
    /* 0x08 */ '&',  '*', '(', ')', '_', '+', '\b','\t',
    /* 0x10 */ 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    /* 0x18 */ 'O',  'P', '{', '}', '\n', 0,  'A', 'S',
    /* 0x20 */ 'D',  'F', 'G', 'H', 'J', 'K', 'L', ':',
    /* 0x28 */ '"',  '~', 0,  '|', 'Z', 'X', 'C', 'V',
    /* 0x30 */ 'B',  'N', 'M', '<', '>', '?', 0,  '*',
    /* 0x38 */ 0,    ' ', 0,   0,
};

#endif /* HELIX_DRIVERS_KEYBOARD_SCANCODES_H */
