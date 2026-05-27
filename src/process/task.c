/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/process/task.c
 *  Module:   process/task
 *  Brief:    PCB allocation, stack-frame priming, lifecycle helpers.
 *
 *  Initial stack layout for a freshly created task (low → high addr):
 *
 *      [esp+ 0]  eflags = 0x202 (IF=1, reserved)  -- popfd target
 *      [esp+ 4]  edi    = 0                       -- popped next
 *      [esp+ 8]  esi    = 0
 *      [esp+12]  ebx    = 0
 *      [esp+16]  ebp    = 0
 *      [esp+20]  task_trampoline    -- return target from context_switch
 *      [esp+24]  entry              -- return target from trampoline's `ret`
 *      [esp+28]  task_exit          -- return target if entry ever returns
 *
 *  When schedule() first picks this task, context_switch loads ESP and
 *  pops the four callee-saved regs + EFLAGS, then `ret`s into the
 *  trampoline which `ret`s into the user-provided entry function.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "task.h"
#include "scheduler.h"

#include "../arch/i386/cpu.h"
#include "../drivers/timer/pit.h"
#include "../lib/printf.h"
#include "../lib/string.h"
#include "../memory/heap.h"

/* Defined in context_switch.asm */
extern void task_trampoline(void);

static uint32_t next_pid = 1;
static task_t  *task_list;    /* circular doubly-linked, always non-empty after init */
static task_t  *current;
static task_t   init_task;    /* PCB for the implicit kmain task */

/* ── List manipulation ──────────────────────────────────────────────── */

static void list_insert(task_t *t)
{
    if (!task_list) {
        t->next = t;
        t->prev = t;
        task_list = t;
        return;
    }
    /* Insert before head (i.e. at the tail of the circular list). */
    t->next = task_list;
    t->prev = task_list->prev;
    task_list->prev->next = t;
    task_list->prev = t;
}

static void list_remove(task_t *t) USED;
static void list_remove(task_t *t)
{
    if (t->next == t) {
        task_list = NULL;
        return;
    }
    t->prev->next = t->next;
    t->next->prev = t->prev;
    if (task_list == t)
        task_list = t->next;
    t->next = t->prev = NULL;
}

/* ── Public API ─────────────────────────────────────────────────────── */

task_t *task_current(void)
{
    return current;
}

void task_set_current(task_t *t)
{
    current = t;
}

void task_init(void)
{
    memset(&init_task, 0, sizeof(init_task));
    strcpy(init_task.name, "init");
    init_task.pid     = 0;
    init_task.state   = TASK_RUNNING;
    init_task.stack   = NULL;     /* boot stack — not heap-managed */
    init_task.esp     = 0;        /* filled in on first context_switch away */

    list_insert(&init_task);
    current = &init_task;

    scheduler_init(current);

    kprintf("[task] init task installed (pid 0)\n");
}

task_t *task_create(const char *name, task_entry_t entry)
{
    task_t *t = (task_t *)kmalloc(sizeof(task_t));
    if (!t)
        return NULL;

    void *stack = kmalloc(TASK_STACK_SIZE);
    if (!stack) {
        kfree(t);
        return NULL;
    }
    memset(stack, 0, TASK_STACK_SIZE);

    memset(t, 0, sizeof(*t));
    strncpy(t->name, name, TASK_NAME_MAX - 1);
    t->pid       = next_pid++;
    t->state     = TASK_READY;
    t->stack     = stack;
    t->wake_tick = 0;

    /* Build the initial saved frame (see header comment for layout).
     * Order on the stack from LOW to HIGH addr must match the pop
     * sequence in context_switch.asm: popfd first, then edi/esi/ebx/ebp,
     * then ret (which pops task_trampoline). */
    uint32_t *sp = (uint32_t *)((uint8_t *)stack + TASK_STACK_SIZE);
    *--sp = (uint32_t)(uintptr_t)task_exit;        /* if entry ever returns */
    *--sp = (uint32_t)(uintptr_t)entry;            /* trampoline ret target */
    *--sp = (uint32_t)(uintptr_t)task_trampoline;  /* context_switch ret target */
    *--sp = 0;                                      /* ebp */
    *--sp = 0;                                      /* ebx */
    *--sp = 0;                                      /* esi */
    *--sp = 0;                                      /* edi */
    *--sp = 0x202;                                  /* EFLAGS: IF=1 (popped FIRST) */
    t->esp = (uint32_t)(uintptr_t)sp;

    cpu_cli();
    list_insert(t);
    cpu_sti();

    kprintf("[task] created '%s' (pid %u, stack=0x%08x, esp=0x%08x)\n",
            t->name, t->pid,
            (uint32_t)(uintptr_t)stack, t->esp);
    return t;
}

void task_yield(void)
{
    schedule();
}

void task_sleep_ms(uint32_t ms)
{
    uint32_t ticks_to_wait = (ms * PIT_FREQ_HZ + 999U) / 1000U;
    cpu_cli();
    current->wake_tick = pit_ticks() + ticks_to_wait;
    current->state     = TASK_SLEEPING;
    cpu_sti();
    schedule();
}

NORETURN void task_exit(void)
{
    cpu_cli();
    kprintf("[task] '%s' (pid %u) exited\n", current->name, current->pid);
    current->state = TASK_ZOMBIE;
    cpu_sti();

    schedule();          /* will not return */
    cpu_hang();
}

void task_wake_due(uint32_t now)
{
    task_t *t = task_list;
    if (!t)
        return;
    do {
        if (t->state == TASK_SLEEPING && (int32_t)(now - t->wake_tick) >= 0)
            t->state = TASK_READY;
        t = t->next;
    } while (t != task_list);
}

void task_for_each(task_visitor_t fn, void *ctx)
{
    task_t *t = task_list;
    if (!t)
        return;
    do {
        fn(t, ctx);
        t = t->next;
    } while (t != task_list);
}
