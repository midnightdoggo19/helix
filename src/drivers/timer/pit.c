/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/timer/pit.c
 *  Module:   drivers/timer
 *  Brief:    PIT channel-0 setup, tick counter, sleep helper.
 *
 *  PIT command byte (port 0x43):
 *      bits 7-6  channel select (00 = ch 0)
 *      bits 5-4  access mode (11 = lobyte then hibyte)
 *      bits 3-1  operating mode (011 = square wave)
 *      bit  0    BCD/binary (0 = binary)
 *
 *  Command we send: 0x36 = 0011 0110.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "pit.h"

#include "../../arch/i386/cpu.h"
#include "../../arch/i386/io.h"
#include "../../arch/i386/irq.h"
#include "../../arch/i386/isr.h"
#include "../../arch/i386/pic.h"
#include "../../lib/printf.h"
#include "../../process/scheduler.h"

#define PIT_DATA0   0x40
#define PIT_CMD     0x43

#define PIT_CMD_BYTE  0x36

static volatile uint32_t ticks;

static void pit_handler(registers_t *regs)
{
    UNUSED(regs);
    ticks++;
    scheduler_tick();
}

void pit_init(void)
{
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ_HZ / PIT_FREQ_HZ);

    outb(PIT_CMD,   PIT_CMD_BYTE);
    outb(PIT_DATA0, (uint8_t)(divisor & 0xFF));
    outb(PIT_DATA0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register(0, pit_handler);
    pic_unmask(0);

    kprintf("[pit ] %u Hz (divisor %u), IRQ 0 unmasked\n",
            PIT_FREQ_HZ, divisor);
}

uint32_t pit_ticks(void)
{
    return ticks;
}

uint32_t pit_uptime_ms(void)
{
    /* PIT_FREQ_HZ = 100  =>  one tick = 10 ms. */
    return ticks * (1000U / PIT_FREQ_HZ);
}

void pit_sleep_ms(uint32_t ms)
{
    uint32_t target = ticks + (ms * PIT_FREQ_HZ + 999U) / 1000U;
    while (ticks < target)
        cpu_halt();   /* wake on next IRQ 0 */
}
