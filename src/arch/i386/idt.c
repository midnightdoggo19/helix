/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/idt.c
 *  Module:   arch/i386/idt
 *  Brief:    Build and load the IDT.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "idt.h"
#include "gdt.h"
#include "../../lib/string.h"

static idt_entry_t idt[IDT_NUM_ENTRIES];
static idt_ptr_t   idt_descriptor;

/* Defined in idt_load.asm */
extern void idt_load(uint32_t idt_ptr_addr);

void idt_set_gate(uint8_t vector, uint32_t handler,
                  uint16_t selector, uint8_t type_attr)
{
    idt[vector].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vector].selector    = selector;
    idt[vector].zero        = 0;
    idt[vector].type_attr   = type_attr;
}

void idt_init(void)
{
    memset(idt, 0, sizeof(idt));

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base  = (uint32_t)(uintptr_t)&idt;

    idt_load((uint32_t)(uintptr_t)&idt_descriptor);
}
