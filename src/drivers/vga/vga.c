/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/vga/vga.c
 *  Module:   drivers/vga
 *  Brief:    80x25 text-mode VGA driver implementation.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "vga.h"
#include "../../arch/i386/io.h"
#include "../../lib/string.h"

#define VGA_BUFFER   ((volatile uint16_t *)0xB8000)

/* CRTC index/data ports for the hardware cursor */
#define CRTC_INDEX   0x3D4
#define CRTC_DATA    0x3D5
#define CURSOR_HI    0x0E
#define CURSOR_LO    0x0F

static uint8_t cursor_x;
static uint8_t cursor_y;
static uint8_t color_attr;

/* ── Internal helpers ───────────────────────────────────────────────── */

static inline uint16_t make_cell(char c, uint8_t attr)
{
    return ((uint16_t)attr << 8) | (uint8_t)c;
}

static void move_hw_cursor(void)
{
    uint16_t pos = (uint16_t)cursor_y * VGA_WIDTH + cursor_x;
    outb(CRTC_INDEX, CURSOR_LO);
    outb(CRTC_DATA,  (uint8_t)(pos & 0xFF));
    outb(CRTC_INDEX, CURSOR_HI);
    outb(CRTC_DATA,  (uint8_t)((pos >> 8) & 0xFF));
}

static void scroll_one_line(void)
{
    /* Shift every row up by one. */
    for (uint16_t i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++)
        VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];

    /* Blank the new bottom row. */
    uint16_t blank = make_cell(' ', color_attr);
    for (uint16_t i = (VGA_HEIGHT - 1) * VGA_WIDTH;
         i < VGA_HEIGHT * VGA_WIDTH;
         i++) {
        VGA_BUFFER[i] = blank;
    }
}

static void newline(void)
{
    cursor_x = 0;
    if (++cursor_y >= VGA_HEIGHT) {
        scroll_one_line();
        cursor_y = VGA_HEIGHT - 1;
    }
}

/* ── Public API ─────────────────────────────────────────────────────── */

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    color_attr = (uint8_t)((bg & 0x0F) << 4 | (fg & 0x0F));
}

void vga_set_cursor(uint8_t x, uint8_t y)
{
    if (x >= VGA_WIDTH)  x = VGA_WIDTH  - 1;
    if (y >= VGA_HEIGHT) y = VGA_HEIGHT - 1;
    cursor_x = x;
    cursor_y = y;
    move_hw_cursor();
}

void vga_clear(void)
{
    uint16_t blank = make_cell(' ', color_attr);
    for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_BUFFER[i] = blank;
    cursor_x = 0;
    cursor_y = 0;
    move_hw_cursor();
}

void vga_init(void)
{
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_putc(char c)
{
    switch (c) {
    case '\n':
        newline();
        break;
    case '\r':
        cursor_x = 0;
        break;
    case '\b':
        if (cursor_x > 0) {
            cursor_x--;
            VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] =
                make_cell(' ', color_attr);
        }
        break;
    case '\t':
        cursor_x = (uint8_t)((cursor_x + 4) & ~3);
        if (cursor_x >= VGA_WIDTH)
            newline();
        break;
    default:
        VGA_BUFFER[cursor_y * VGA_WIDTH + cursor_x] =
            make_cell(c, color_attr);
        if (++cursor_x >= VGA_WIDTH)
            newline();
        break;
    }
    move_hw_cursor();
}

void vga_puts(const char *s)
{
    while (*s)
        vga_putc(*s++);
}
