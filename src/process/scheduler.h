/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/process/scheduler.h
 *  Module:   process/scheduler
 *  Brief:    Round-Robin scheduler driven by the PIT.
 *
 *  Public entry points:
 *      scheduler_init()    Wired by task_init().
 *      schedule()          Pick next runnable task and switch.
 *      scheduler_tick()    Called by the PIT handler each tick.
 *
 *  The PIT handler in src/drivers/timer/pit.c calls scheduler_tick()
 *  unconditionally; before scheduler_init() it's a no-op.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_PROCESS_SCHEDULER_H
#define HELIX_PROCESS_SCHEDULER_H

#include "task.h"

/** Mark the scheduler ready; called from task_init(). */
void scheduler_init(task_t *initial_current);

/** Pick the next ready task and switch to it (may not return). */
void schedule(void);

/** Per-PIT-tick callback. Wakes due sleepers and may trigger preemption. */
void scheduler_tick(void);

/**
 * Assembly: save callee-saved regs + EFLAGS to current task's stack,
 * record ESP into *old_esp, load ESP from new_esp, restore, ret.
 *
 * Implemented in context_switch.asm.
 */
extern void context_switch(uint32_t *old_esp_ptr, uint32_t new_esp);

#endif /* HELIX_PROCESS_SCHEDULER_H */
