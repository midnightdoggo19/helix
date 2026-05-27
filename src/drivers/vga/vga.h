/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/vga/vga.h
 *  Module:   drivers/vga
 *  Brief:    80x25 VGA text-mode console driver.
 *
 *  Memory-mapped at the legacy physical address 0xB8000. Each cell is
 *  a 16-bit word: low byte = ASCII codepoint, high byte = attribute
 *  (background<<4 | foreground).
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_DRIVERS_VGA_H
#define HELIX_DRIVERS_VGA_H

#include <helix/types.h>

#define VGA_WIDTH   80
#define VGA_HEIGHT  25

typedef enum {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW        = 14,
    VGA_WHITE         = 15,
} vga_color_t;

/** Clear screen, reset cursor, install default colors. */
void vga_init(void);

/** Erase the screen with the current background color. */
void vga_clear(void);

/** Write one character. Handles '\n', '\r', '\b', and scrolling. */
void vga_putc(char c);

/** Write a null-terminated string. */
void vga_puts(const char *s);

/** Change foreground/background for subsequent characters. */
void vga_set_color(vga_color_t fg, vga_color_t bg);

/** Move the hardware cursor to (x, y). */
void vga_set_cursor(uint8_t x, uint8_t y);

#endif /* HELIX_DRIVERS_VGA_H */
