/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/lib/printf.c
 *  Module:   lib/printf
 *  Brief:    Format string engine for the kernel console.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "printf.h"
#include "string.h"
#include "../drivers/serial/serial.h"
#include "../drivers/vga/vga.h"

/* ── Internal helpers ───────────────────────────────────────────────── */

static int emit_str(putc_fn_t sink, const char *s, int width, bool left_align)
{
    int len = (int)strlen(s);
    int pad = width > len ? width - len : 0;
    int n   = 0;

    if (!left_align)
        while (pad--) { sink(' '); n++; }

    while (*s) { sink(*s++); n++; }

    if (left_align)
        while (pad--) { sink(' '); n++; }

    return n;
}

static int emit_uint(putc_fn_t sink, uint32_t value, int base, bool upper,
                     int width, bool zero_pad)
{
    /* Max 10 digits for 32-bit decimal, 8 for hex. Buffer of 16 covers both. */
    char buf[16];
    int  i = 0;
    int  n = 0;

    const char *digits = upper ? "0123456789ABCDEF"
                               : "0123456789abcdef";

    if (value == 0) {
        buf[i++] = '0';
    } else {
        while (value && i < (int)sizeof(buf)) {
            buf[i++] = digits[value % (uint32_t)base];
            value /= (uint32_t)base;
        }
    }

    int pad = width > i ? width - i : 0;
    char pad_char = zero_pad ? '0' : ' ';
    while (pad--) { sink(pad_char); n++; }

    while (i--) { sink(buf[i]); n++; }
    return n;
}

static int emit_int(putc_fn_t sink, int32_t value, int width, bool zero_pad)
{
    int n = 0;
    if (value < 0) {
        sink('-');
        n++;
        if (width > 0) width--;
        return n + emit_uint(sink, (uint32_t)(-value), 10, false, width, zero_pad);
    }
    return emit_uint(sink, (uint32_t)value, 10, false, width, zero_pad);
}

/* ── Core formatter ─────────────────────────────────────────────────── */

int kvprintf(putc_fn_t sink, const char *fmt, __builtin_va_list ap)
{
    int n = 0;

    while (*fmt) {
        if (*fmt != '%') {
            sink(*fmt++);
            n++;
            continue;
        }

        fmt++; /* skip '%' */

        bool left_align = false;
        bool zero_pad   = false;
        int  width      = 0;

        /* Flag */
        if (*fmt == '-') { left_align = true; fmt++; }
        if (*fmt == '0') { zero_pad   = true; fmt++; }

        /* Width */
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* Conversion */
        switch (*fmt) {
        case 'c': {
            char c = (char)__builtin_va_arg(ap, int);
            sink(c);
            n++;
            break;
        }
        case 's': {
            const char *s = __builtin_va_arg(ap, const char *);
            if (!s) s = "(null)";
            n += emit_str(sink, s, width, left_align);
            break;
        }
        case 'd':
        case 'i': {
            int32_t v = __builtin_va_arg(ap, int32_t);
            n += emit_int(sink, v, width, zero_pad);
            break;
        }
        case 'u': {
            uint32_t v = __builtin_va_arg(ap, uint32_t);
            n += emit_uint(sink, v, 10, false, width, zero_pad);
            break;
        }
        case 'x': {
            uint32_t v = __builtin_va_arg(ap, uint32_t);
            n += emit_uint(sink, v, 16, false, width, zero_pad);
            break;
        }
        case 'X': {
            uint32_t v = __builtin_va_arg(ap, uint32_t);
            n += emit_uint(sink, v, 16, true, width, zero_pad);
            break;
        }
        case 'p': {
            uintptr_t v = (uintptr_t)__builtin_va_arg(ap, void *);
            sink('0'); sink('x'); n += 2;
            n += emit_uint(sink, (uint32_t)v, 16, false, 8, true);
            break;
        }
        case '%': {
            sink('%');
            n++;
            break;
        }
        default: {
            /* Unknown conversion — pass through literally. */
            sink('%');
            sink(*fmt);
            n += 2;
            break;
        }
        }

        if (*fmt) fmt++;
    }

    return n;
}

/* ── Public sinks ───────────────────────────────────────────────────── */

void kputc(char c)
{
    vga_putc(c);
    serial_putc(c);
}

void kputs(const char *s)
{
    while (*s)
        kputc(*s++);
}

int kprintf(const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int n = kvprintf(kputc, fmt, ap);
    __builtin_va_end(ap);
    return n;
}
