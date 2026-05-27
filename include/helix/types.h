/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     include/helix/types.h
 *  Module:   core/types
 *  Brief:    Fixed-width integer types and core typedefs.
 *
 *  Freestanding environments do not ship the hosted libc headers, so
 *  we declare every primitive type we rely on here. Keeping this in
 *  one place means a future port (x86_64, ARM) only updates this file.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_TYPES_H
#define HELIX_TYPES_H

/* ── Fixed-width integers ───────────────────────────────────────────── */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

/* ── Pointer-sized integers (32-bit on i386) ────────────────────────── */
typedef uint32_t            uintptr_t;
typedef int32_t             intptr_t;

/* ── Size types ─────────────────────────────────────────────────────── */
typedef uint32_t            size_t;
typedef int32_t             ssize_t;

/* ── Boolean ────────────────────────────────────────────────────────── */
typedef _Bool               bool;
#define true                ((bool)1)
#define false               ((bool)0)

/* ── Null pointer ───────────────────────────────────────────────────── */
#ifndef NULL
#define NULL                ((void *)0)
#endif

/* ── Limits ─────────────────────────────────────────────────────────── */
#define UINT8_MAX           0xFFU
#define UINT16_MAX          0xFFFFU
#define UINT32_MAX          0xFFFFFFFFU
#define UINT64_MAX          0xFFFFFFFFFFFFFFFFULL

#endif /* HELIX_TYPES_H */
