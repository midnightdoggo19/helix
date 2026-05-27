/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/process/sync.c
 *  Module:   process/sync
 * ─────────────────────────────────────────────────────────────────────
 */
#include "sync.h"
#include "task.h"

#include "../arch/i386/cpu.h"

/* ── Spinlock ───────────────────────────────────────────────────────── */

void spin_lock(spinlock_t *l)
{
    uint32_t flags = cpu_read_eflags();
    cpu_cli();
    l->saved_eflags = flags;
    l->held = true;
}

void spin_unlock(spinlock_t *l)
{
    uint32_t flags = l->saved_eflags;
    l->held = false;
    if (flags & 0x200) /* IF was set on the way in */
        cpu_sti();
}

/* ── Mutex ──────────────────────────────────────────────────────────── */

void mutex_lock(mutex_t *m)
{
    for (;;) {
        cpu_cli();
        if (!m->locked) {
            m->locked = true;
            m->owner  = task_current();
            cpu_sti();
            return;
        }
        cpu_sti();
        task_yield();
    }
}

void mutex_unlock(mutex_t *m)
{
    cpu_cli();
    m->locked = false;
    m->owner  = NULL;
    cpu_sti();
}

/* ── Semaphore ──────────────────────────────────────────────────────── */

void sem_init(semaphore_t *s, int32_t initial)
{
    s->count = initial;
}

void sem_wait(semaphore_t *s)
{
    for (;;) {
        cpu_cli();
        if (s->count > 0) {
            s->count--;
            cpu_sti();
            return;
        }
        cpu_sti();
        task_yield();
    }
}

void sem_signal(semaphore_t *s)
{
    cpu_cli();
    s->count++;
    cpu_sti();
}
