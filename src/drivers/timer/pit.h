/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/timer/pit.h
 *  Module:   drivers/timer
 *  Brief:    8253/8254 Programmable Interval Timer driver.
 *
 *  Channel 0 is wired to IRQ 0. The PIT base frequency is 1.193182 MHz;
 *  we divide it down to `PIT_FREQ_HZ` (default 100) to get a 10 ms tick.
 *
 *  Public surface:
 *      pit_init()          — program the timer + register IRQ 0
 *      pit_ticks()         — monotonic tick count since boot
 *      pit_uptime_ms()     — milliseconds since boot
 *      pit_sleep_ms(ms)    — block until @p ms have elapsed (uses HLT)
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_DRIVERS_TIMER_PIT_H
#define HELIX_DRIVERS_TIMER_PIT_H

#include <helix/types.h>

#define PIT_BASE_FREQ_HZ   1193182U
#define PIT_FREQ_HZ        100U          /* 10 ms / tick */

void     pit_init(void);
uint32_t pit_ticks(void);
uint32_t pit_uptime_ms(void);
void     pit_sleep_ms(uint32_t ms);

#endif /* HELIX_DRIVERS_TIMER_PIT_H */
