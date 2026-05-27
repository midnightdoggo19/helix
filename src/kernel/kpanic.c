/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/kernel/kpanic.c
 *  Module:   kernel/panic
 *  Brief:    Fatal-error reporting and CPU halt.
 *
 *  kpanic() prints a red banner with the failure message, dumps it
 *  to serial as well, then disables interrupts and halts forever.
 *  It is the *only* function in the kernel allowed to never return.
 * ─────────────────────────────────────────────────────────────────────
 */
#include <helix/kernel.h>

#include "../arch/i386/cpu.h"
#include "../drivers/serial/serial.h"
#include "../drivers/vga/vga.h"
#include "../lib/printf.h"

NORETURN void kpanic(const char *fmt, ...)
{
    cpu_cli();

    vga_set_color(VGA_WHITE, VGA_RED);
    kputs("\n[ KERNEL PANIC ] ");

    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    kvprintf(kputc, fmt, ap);
    __builtin_va_end(ap);

    kputs("\nSystem halted.\n");
    cpu_hang();
}
