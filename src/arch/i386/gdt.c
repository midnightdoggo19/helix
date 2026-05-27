/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/gdt.c
 *  Module:   arch/i386/gdt
 *  Brief:    GDT entry packing and installation.
 *
 *  Access byte layout (bit 7 .. bit 0):
 *      P  | DPL | DPL | S | E | DC | RW | A
 *      P  : Present
 *      DPL: Descriptor Privilege Level (0 = ring 0, 3 = ring 3)
 *      S  : Descriptor type (1 = code/data, 0 = system)
 *      E  : Executable
 *      DC : Direction / Conforming
 *      RW : Readable (code) or Writable (data)
 *      A  : Accessed (CPU sets this)
 *
 *  Granularity byte layout:
 *      G | DB | L | AVL | limit[19:16]
 *      G  : 1 = 4 KiB pages, 0 = bytes
 *      DB : 1 = 32-bit segment
 *      L  : 64-bit (always 0 here)
 *      AVL: software use
 * ─────────────────────────────────────────────────────────────────────
 */
#include "gdt.h"
#include "../../lib/string.h"

static gdt_entry_t gdt[GDT_NUM_ENTRIES];
static gdt_ptr_t   gdt_descriptor;

/* Defined in gdt_flush.asm */
extern void gdt_flush(uint32_t gdt_ptr_addr);

static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t granularity)
{
    gdt[i].base_low    = (uint16_t)(base & 0xFFFF);
    gdt[i].base_mid    = (uint8_t)((base >> 16) & 0xFF);
    gdt[i].base_high   = (uint8_t)((base >> 24) & 0xFF);

    gdt[i].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt[i].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt[i].granularity |= (granularity & 0xF0);

    gdt[i].access = access;
}

void gdt_init(void)
{
    /* 0: null segment (CPU requirement) */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* 1: kernel code  — 0x9A access, 0xCF granularity */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xC0);

    /* 2: kernel data  — 0x92 access */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    /* 3: user code    — 0xFA access (ring 3) */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xC0);

    /* 4: user data    — 0xF2 access (ring 3) */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);

    gdt_descriptor.limit = sizeof(gdt) - 1;
    gdt_descriptor.base  = (uint32_t)(uintptr_t)&gdt;

    gdt_flush((uint32_t)(uintptr_t)&gdt_descriptor);
}
