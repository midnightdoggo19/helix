/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/lib/printf.h
 *  Module:   lib/printf
 *  Brief:    Freestanding printf for the kernel console.
 *
 *  Supported conversions:
 *      %c        single character
 *      %s        null-terminated string
 *      %d / %i   signed decimal
 *      %u        unsigned decimal
 *      %x / %X   hexadecimal (lower / upper case)
 *      %p        pointer (formatted as 0x%08x)
 *      %%        literal '%'
 *
 *  Width and zero-padding flags are supported, e.g. "%08x" or "%5d".
 *  Floating point is intentionally NOT supported — no FPU in kernel.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_LIB_PRINTF_H
#define HELIX_LIB_PRINTF_H

#include <helix/types.h>

/** Output sink — called once per character by the formatter. */
typedef void (*putc_fn_t)(char c);

/**
 * Format a string and emit it character-by-character via @p sink.
 * Returns the number of characters emitted.
 */
int kvprintf(putc_fn_t sink, const char *fmt, __builtin_va_list ap);

/**
 * Format a string to the system kernel console (VGA + serial).
 * Returns the number of characters emitted.
 */
int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/** Write a single character to the kernel console. */
void kputc(char c);

/** Write a null-terminated string to the kernel console. */
void kputs(const char *s);

#endif /* HELIX_LIB_PRINTF_H */
