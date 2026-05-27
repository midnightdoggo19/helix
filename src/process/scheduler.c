/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/process/scheduler.c
 *  Module:   process/scheduler
 *  Brief:    Round-Robin selection over the all-tasks circular list.
 *
 *  The scheduler does NOT keep its own `current` pointer — it routes
 *  every read through task_current() and every write through
 *  task_set_current(). This avoids a previous bug where two static
 *  variables of the same name in different translation units became
 *  unsynchronized.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "scheduler.h"

#include "../arch/i386/cpu.h"
#include "../drivers/timer/pit.h"
#include "../lib/printf.h"

static bool sched_ready;

void scheduler_init(task_t *initial_current)
{
    task_set_current(initial_current);
    sched_ready = true;
    kprintf("[sched] round-robin scheduler online\n");
}

/* Return the next runnable task. Never NULL (init is always runnable). */
static task_t *pick_next(void)
{
    task_t *cur   = task_current();
    task_t *start = cur->next;
    task_t *t     = start;

    do {
        if (t->state == TASK_READY)
            return t;
        t = t->next;
    } while (t != start);

    /* No other ready task — keep current if it can still run. */
    if (cur->state == TASK_RUNNING || cur->state == TASK_READY)
        return cur;

    /* Current can't run and nobody else is ready: idle until something
     * wakes up (PIT IRQ fires here naturally). */
    while (1) {
        cpu_sti();
        cpu_halt();
        cpu_cli();
        t = start;
        do {
            if (t->state == TASK_READY)
                return t;
            t = t->next;
        } while (t != start);
    }
}

void schedule(void)
{
    if (!sched_ready)
        return;

    cpu_cli();

    task_t *prev = task_current();
    task_t *next = pick_next();

    if (next == prev) {
        if (prev->state == TASK_READY)
            prev->state = TASK_RUNNING;
        cpu_sti();
        return;
    }

    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;
    next->state = TASK_RUNNING;

    task_set_current(next);

    /* EFLAGS (incl. IF) is restored from next task's frame by popfd. */
    context_switch(&prev->esp, next->esp);
}

void scheduler_tick(void)
{
    if (!sched_ready)
        return;

    task_wake_due(pit_ticks());
    schedule();
}
