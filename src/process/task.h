/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/process/task.h
 *  Module:   process/task
 *  Brief:    Task Control Block + lifecycle API.
 *
 *  HelixOS uses kernel threads only (no user mode yet). A task owns a
 *  kernel stack and a saved ESP. When preempted, the scheduler stashes
 *  callee-saved registers on the task's stack and records ESP in the
 *  PCB; restoring is the reverse.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_PROCESS_TASK_H
#define HELIX_PROCESS_TASK_H

#include <helix/kernel.h>

#define TASK_STACK_SIZE   8192
#define TASK_NAME_MAX     32

typedef enum {
    TASK_NEW,        /* allocated but not yet scheduled */
    TASK_READY,      /* on the run queue, waiting for CPU */
    TASK_RUNNING,    /* currently using the CPU */
    TASK_SLEEPING,   /* blocked on a wake_tick deadline */
    TASK_ZOMBIE,     /* exited, awaiting reaping */
} task_state_t;

typedef void (*task_entry_t)(void);

typedef struct task {
    uint32_t       esp;                 /* saved kernel ESP */
    uint32_t       pid;
    char           name[TASK_NAME_MAX];
    task_state_t   state;
    void          *stack;               /* base of allocated stack (for free) */
    uint32_t       wake_tick;           /* if SLEEPING: tick to wake at */
    struct task   *next;                /* all-tasks circular list */
    struct task   *prev;
} task_t;

/** Bring up the task subsystem. Installs the implicit "init" task. */
void task_init(void);

/** Create a new task in READY state. Returns the new PCB or NULL. */
task_t *task_create(const char *name, task_entry_t entry);

/** Currently running task. Never NULL after task_init(). */
task_t *task_current(void);

/** Update the current-task pointer (used by the scheduler on switch). */
void task_set_current(task_t *t);

/** Voluntarily yield the CPU. */
void task_yield(void);

/** Block the caller until at least @p ms have passed. */
void task_sleep_ms(uint32_t ms);

/** Terminate the current task and never return. */
NORETURN void task_exit(void);

/** Wake any SLEEPING tasks whose wake_tick has been reached. */
void task_wake_due(uint32_t current_tick);

/** Iterate the all-tasks list (for debugging / `ps`-style output). */
typedef void (*task_visitor_t)(const task_t *t, void *ctx);
void task_for_each(task_visitor_t fn, void *ctx);

#endif /* HELIX_PROCESS_TASK_H */
