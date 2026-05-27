/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/irq.c
 *  Module:   arch/i386/irq
 *  Brief:    Install IRQ gates, dispatch to driver handlers, EOI.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "irq.h"
#include "idt.h"
#include "gdt.h"
#include "pic.h"

/* External symbols from irq_stubs.asm */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

static int_handler_t handlers[16];

void irq_register(uint8_t irq, int_handler_t handler)
{
    if (irq < 16)
        handlers[irq] = handler;
}

void irq_unregister(uint8_t irq)
{
    if (irq < 16)
        handlers[irq] = NULL;
}

void irq_dispatch(registers_t *regs)
{
    uint8_t irq = (uint8_t)(regs->int_no - 32);

    /* Acknowledge the PIC BEFORE invoking the handler. The handler may
     * call schedule() which context-switches away — if we EOI after,
     * the IRQ stays "in service" on the PIC and lower-priority IRQs
     * (such as IRQ 1 keyboard) are blocked. Interrupts are still
     * disabled at this point (cli at stub entry), so this is safe. */
    pic_send_eoi(irq);

    if (irq < 16 && handlers[irq])
        handlers[irq](regs);
}

void irq_install(void)
{
    const uint8_t flags = IDT_GATE_PRESENT | IDT_GATE_DPL0 | IDT_GATE_INT32;

    idt_set_gate(32, (uint32_t)(uintptr_t)irq0,  GDT_KERNEL_CODE, flags);
    idt_set_gate(33, (uint32_t)(uintptr_t)irq1,  GDT_KERNEL_CODE, flags);
    idt_set_gate(34, (uint32_t)(uintptr_t)irq2,  GDT_KERNEL_CODE, flags);
    idt_set_gate(35, (uint32_t)(uintptr_t)irq3,  GDT_KERNEL_CODE, flags);
    idt_set_gate(36, (uint32_t)(uintptr_t)irq4,  GDT_KERNEL_CODE, flags);
    idt_set_gate(37, (uint32_t)(uintptr_t)irq5,  GDT_KERNEL_CODE, flags);
    idt_set_gate(38, (uint32_t)(uintptr_t)irq6,  GDT_KERNEL_CODE, flags);
    idt_set_gate(39, (uint32_t)(uintptr_t)irq7,  GDT_KERNEL_CODE, flags);
    idt_set_gate(40, (uint32_t)(uintptr_t)irq8,  GDT_KERNEL_CODE, flags);
    idt_set_gate(41, (uint32_t)(uintptr_t)irq9,  GDT_KERNEL_CODE, flags);
    idt_set_gate(42, (uint32_t)(uintptr_t)irq10, GDT_KERNEL_CODE, flags);
    idt_set_gate(43, (uint32_t)(uintptr_t)irq11, GDT_KERNEL_CODE, flags);
    idt_set_gate(44, (uint32_t)(uintptr_t)irq12, GDT_KERNEL_CODE, flags);
    idt_set_gate(45, (uint32_t)(uintptr_t)irq13, GDT_KERNEL_CODE, flags);
    idt_set_gate(46, (uint32_t)(uintptr_t)irq14, GDT_KERNEL_CODE, flags);
    idt_set_gate(47, (uint32_t)(uintptr_t)irq15, GDT_KERNEL_CODE, flags);
}
