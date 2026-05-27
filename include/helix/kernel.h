/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     include/helix/kernel.h
 *  Module:   core/kernel
 *  Brief:    Kernel-wide macros, attributes, and panic interface.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_KERNEL_H
#define HELIX_KERNEL_H

#include <helix/types.h>
#include <helix/version.h>

/* ── Compile-time helpers ───────────────────────────────────────────── */
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define ALIGN_UP(x, a)      (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))
#define IS_ALIGNED(x, a)    (((x) & ((a) - 1)) == 0)
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define UNUSED(x)           ((void)(x))

/* ── GCC attribute shorthands ───────────────────────────────────────── */
#define PACKED              __attribute__((packed))
#define ALIGNED(n)          __attribute__((aligned(n)))
#define NORETURN            __attribute__((noreturn))
#define USED                __attribute__((used))
#define INLINE              __attribute__((always_inline)) inline
#define COLD                __attribute__((cold))

/* ── Panic ──────────────────────────────────────────────────────────── */
NORETURN void kpanic(const char *fmt, ...);

#define KASSERT(cond)                                                       \
    do {                                                                    \
        if (!(cond))                                                        \
            kpanic("KASSERT failed: %s  (at %s:%d)",                        \
                   #cond, __FILE__, __LINE__);                              \
    } while (0)

#endif /* HELIX_KERNEL_H */
