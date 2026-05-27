/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/process/sync.h
 *  Module:   process/sync
 *  Brief:    Spinlock, mutex, semaphore.
 *
 *  HelixOS is single-CPU. "Spinlock" is implemented as save-and-cli;
 *  no actual spin is required because there's no other CPU to wait
 *  for. The cli prevents the IRQ that drives preemption from running,
 *  so a critical section is atomic with respect to all kernel code.
 *
 *  Mutex and semaphore yield instead of busy-spinning, so a blocked
 *  task gives the CPU to others.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_PROCESS_SYNC_H
#define HELIX_PROCESS_SYNC_H

#include <helix/kernel.h>

/* ── Spinlock ───────────────────────────────────────────────────────── */

typedef struct {
    uint32_t saved_eflags;
    bool     held;
} spinlock_t;

#define SPINLOCK_INIT  { 0, false }

void spin_lock(spinlock_t *l);
void spin_unlock(spinlock_t *l);

/* ── Mutex (yielding) ───────────────────────────────────────────────── */

struct task;

typedef struct {
    volatile bool locked;
    struct task  *owner;
} mutex_t;

#define MUTEX_INIT  { false, NULL }

void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

/* ── Counting semaphore (yielding) ──────────────────────────────────── */

typedef struct {
    volatile int32_t count;
} semaphore_t;

#define SEMAPHORE_INIT(n)  { (n) }

void sem_init(semaphore_t *s, int32_t initial);
void sem_wait(semaphore_t *s);     /* down: block until count > 0 */
void sem_signal(semaphore_t *s);   /* up:   increment count */

#endif /* HELIX_PROCESS_SYNC_H */
