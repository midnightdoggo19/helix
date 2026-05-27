/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/isr.c
 *  Module:   arch/i386/isr
 *  Brief:    Install ISR gates, hold the handler table, dispatch.
 *
 *  The 32 exception stubs in isr_stubs.asm all jump to isr_common
 *  which calls isr_dispatch(). This function looks up regs->int_no
 *  and invokes the registered C handler (or the default fatal handler
 *  if none has been installed).
 * ─────────────────────────────────────────────────────────────────────
 */
#include "isr.h"
#include "idt.h"
#include "gdt.h"

#include <helix/kernel.h>
#include "../../lib/printf.h"

/* External symbols from isr_stubs.asm */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

static int_handler_t handlers[32];

static const char *exception_names[32] = {
    "Divide-by-zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPU error",
    "Alignment check",
    "Machine check",
    "SIMD FP exception",
    "Virtualization exception",
    "Control-protection exception",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved",
};

void isr_register(uint8_t vector, int_handler_t handler)
{
    if (vector < 32)
        handlers[vector] = handler;
}

static void default_handler(registers_t *r)
{
    kprintf("\n[ EXCEPTION ] vector=%u (%s)  err=0x%x\n",
            r->int_no, exception_names[r->int_no], r->err_code);
    kprintf(" eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
            r->eax, r->ebx, r->ecx, r->edx);
    kprintf(" esi=%08x edi=%08x ebp=%08x esp=%08x\n",
            r->esi, r->edi, r->ebp, r->esp_dummy);
    kprintf(" eip=%08x cs=%04x eflags=%08x ds=%04x\n",
            r->eip, r->cs, r->eflags, r->ds);
    kpanic("Unhandled CPU exception");
}

void isr_dispatch(registers_t *regs)
{
    if (regs->int_no < 32 && handlers[regs->int_no]) {
        handlers[regs->int_no](regs);
    } else {
        default_handler(regs);
    }
}

void isr_install(void)
{
    const uint8_t flags = IDT_GATE_PRESENT | IDT_GATE_DPL0 | IDT_GATE_INT32;

    idt_set_gate(0,  (uint32_t)(uintptr_t)isr0,  GDT_KERNEL_CODE, flags);
    idt_set_gate(1,  (uint32_t)(uintptr_t)isr1,  GDT_KERNEL_CODE, flags);
    idt_set_gate(2,  (uint32_t)(uintptr_t)isr2,  GDT_KERNEL_CODE, flags);
    idt_set_gate(3,  (uint32_t)(uintptr_t)isr3,  GDT_KERNEL_CODE, flags);
    idt_set_gate(4,  (uint32_t)(uintptr_t)isr4,  GDT_KERNEL_CODE, flags);
    idt_set_gate(5,  (uint32_t)(uintptr_t)isr5,  GDT_KERNEL_CODE, flags);
    idt_set_gate(6,  (uint32_t)(uintptr_t)isr6,  GDT_KERNEL_CODE, flags);
    idt_set_gate(7,  (uint32_t)(uintptr_t)isr7,  GDT_KERNEL_CODE, flags);
    idt_set_gate(8,  (uint32_t)(uintptr_t)isr8,  GDT_KERNEL_CODE, flags);
    idt_set_gate(9,  (uint32_t)(uintptr_t)isr9,  GDT_KERNEL_CODE, flags);
    idt_set_gate(10, (uint32_t)(uintptr_t)isr10, GDT_KERNEL_CODE, flags);
    idt_set_gate(11, (uint32_t)(uintptr_t)isr11, GDT_KERNEL_CODE, flags);
    idt_set_gate(12, (uint32_t)(uintptr_t)isr12, GDT_KERNEL_CODE, flags);
    idt_set_gate(13, (uint32_t)(uintptr_t)isr13, GDT_KERNEL_CODE, flags);
    idt_set_gate(14, (uint32_t)(uintptr_t)isr14, GDT_KERNEL_CODE, flags);
    idt_set_gate(15, (uint32_t)(uintptr_t)isr15, GDT_KERNEL_CODE, flags);
    idt_set_gate(16, (uint32_t)(uintptr_t)isr16, GDT_KERNEL_CODE, flags);
    idt_set_gate(17, (uint32_t)(uintptr_t)isr17, GDT_KERNEL_CODE, flags);
    idt_set_gate(18, (uint32_t)(uintptr_t)isr18, GDT_KERNEL_CODE, flags);
    idt_set_gate(19, (uint32_t)(uintptr_t)isr19, GDT_KERNEL_CODE, flags);
    idt_set_gate(20, (uint32_t)(uintptr_t)isr20, GDT_KERNEL_CODE, flags);
    idt_set_gate(21, (uint32_t)(uintptr_t)isr21, GDT_KERNEL_CODE, flags);
    idt_set_gate(22, (uint32_t)(uintptr_t)isr22, GDT_KERNEL_CODE, flags);
    idt_set_gate(23, (uint32_t)(uintptr_t)isr23, GDT_KERNEL_CODE, flags);
    idt_set_gate(24, (uint32_t)(uintptr_t)isr24, GDT_KERNEL_CODE, flags);
    idt_set_gate(25, (uint32_t)(uintptr_t)isr25, GDT_KERNEL_CODE, flags);
    idt_set_gate(26, (uint32_t)(uintptr_t)isr26, GDT_KERNEL_CODE, flags);
    idt_set_gate(27, (uint32_t)(uintptr_t)isr27, GDT_KERNEL_CODE, flags);
    idt_set_gate(28, (uint32_t)(uintptr_t)isr28, GDT_KERNEL_CODE, flags);
    idt_set_gate(29, (uint32_t)(uintptr_t)isr29, GDT_KERNEL_CODE, flags);
    idt_set_gate(30, (uint32_t)(uintptr_t)isr30, GDT_KERNEL_CODE, flags);
    idt_set_gate(31, (uint32_t)(uintptr_t)isr31, GDT_KERNEL_CODE, flags);
}
