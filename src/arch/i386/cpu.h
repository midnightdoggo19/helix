/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/arch/i386/cpu.h
 *  Module:   arch/i386/cpu
 *  Brief:    Inline wrappers around CPU control instructions.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_ARCH_I386_CPU_H
#define HELIX_ARCH_I386_CPU_H

#include <helix/kernel.h>

/** Halt the CPU until the next interrupt arrives (low-power idle). */
static inline void cpu_halt(void)
{
    __asm__ volatile("hlt");
}

/** Disable interrupts (CLI). */
static inline void cpu_cli(void)
{
    __asm__ volatile("cli");
}

/** Enable interrupts (STI). */
static inline void cpu_sti(void)
{
    __asm__ volatile("sti");
}

/** Spin forever with interrupts disabled. The classic "we're done" state. */
static inline NORETURN void cpu_hang(void)
{
    for (;;) {
        cpu_cli();
        cpu_halt();
    }
}

/** Read the current EFLAGS register. */
static inline uint32_t cpu_read_eflags(void)
{
    uint32_t flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
    return flags;
}

#endif /* HELIX_ARCH_I386_CPU_H */
