/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/gdt.h
 *  Module:   arch/i386/gdt
 *  Brief:    Global Descriptor Table setup (flat memory model).
 *
 *  HelixOS uses a minimal flat GDT with five entries:
 *      0x00  null              — required by hardware
 *      0x08  kernel code       — ring 0, executable, 4 GiB
 *      0x10  kernel data       — ring 0, writable,   4 GiB
 *      0x18  user   code       — ring 3, executable, 4 GiB
 *      0x20  user   data       — ring 3, writable,   4 GiB
 *
 *  Segment selectors are the byte offsets above (with RPL bits in
 *  the low 2 bits if needed).
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_GDT_H
#define HELIX_ARCH_I386_GDT_H

#include <helix/kernel.h>

#define GDT_NUM_ENTRIES   5

#define GDT_KERNEL_CODE   0x08
#define GDT_KERNEL_DATA   0x10
#define GDT_USER_CODE     0x18
#define GDT_USER_DATA     0x20

/** One 8-byte GDT entry. */
typedef struct PACKED gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;     /* 4 bits flags | 4 bits limit_high */
    uint8_t  base_high;
} gdt_entry_t;

/** Operand of the LGDT instruction. */
typedef struct PACKED gdt_ptr {
    uint16_t limit;
    uint32_t base;
} gdt_ptr_t;

/** Build and install the GDT. After return, segment registers are reloaded. */
void gdt_init(void);

#endif /* HELIX_ARCH_I386_GDT_H */
