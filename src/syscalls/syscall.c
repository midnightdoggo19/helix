/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/syscalls/syscall.c
 *  Module:   syscalls
 *  Brief:    C dispatcher behind INT 0x80 + syscall implementations.
 *
 *  Layout (mirrors registers_t):
 *      r->eax = syscall number on entry, return value on exit
 *      r->ebx = arg0, r->ecx = arg1, r->edx = arg2
 *      r->esi = arg3, r->edi = arg4
 * ─────────────────────────────────────────────────────────────────────
 */
#include "syscall.h"

#include "../arch/i386/gdt.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/isr.h"
#include "../drivers/timer/pit.h"
#include "../drivers/vga/vga.h"
#include "../lib/printf.h"
#include "../lib/string.h"
#include "../memory/heap.h"
#include "../memory/pmm.h"
#include "../process/task.h"
#include "../process/scheduler.h"
#include "../drivers/keyboard/keyboard.h"

extern void syscall_entry(void);

/* ── Per-call handlers ──────────────────────────────────────────────── */

static int32_t sys_exit(registers_t *r)
{
    UNUSED(r);
    task_exit();
    return 0; /* unreachable */
}

static int32_t sys_write(registers_t *r)
{
    uint32_t       fd    = r->ebx;
    const char    *buf   = (const char *)(uintptr_t)r->ecx;
    uint32_t       count = r->edx;

    if (fd != FD_STDOUT && fd != FD_STDERR)
        return -1;
    if (!buf)
        return -1;
    for (uint32_t i = 0; i < count; i++)
        kputc(buf[i]);
    return (int32_t)count;
}

static int32_t sys_read(registers_t *r)
{
    uint32_t  fd    = r->ebx;
    char     *buf   = (char *)(uintptr_t)r->ecx;
    uint32_t  count = r->edx;

    if (fd != FD_STDIN || !buf || count == 0)
        return -1;

    /* Blocking line read terminated by '\n' or buffer-full. */
    uint32_t i = 0;
    while (i < count) {
        char c = kbd_getchar();
        buf[i++] = c;
        if (c == '\n')
            break;
    }
    return (int32_t)i;
}

static int32_t sys_getpid(registers_t *r)
{
    UNUSED(r);
    return (int32_t)task_current()->pid;
}

static int32_t sys_yield(registers_t *r)
{
    UNUSED(r);
    task_yield();
    return 0;
}

static int32_t sys_sleep_ms(registers_t *r)
{
    task_sleep_ms(r->ebx);
    return 0;
}

static int32_t sys_uptime(registers_t *r)
{
    UNUSED(r);
    return (int32_t)pit_uptime_ms();
}

/* sys_ps writes "pid name state\n" lines into the caller's buffer. */
struct ps_ctx { char *buf; size_t cap; size_t len; };

static const char *state_name(task_state_t s)
{
    switch (s) {
    case TASK_NEW:      return "NEW";
    case TASK_READY:    return "READY";
    case TASK_RUNNING:  return "RUN";
    case TASK_SLEEPING: return "SLEEP";
    case TASK_ZOMBIE:   return "ZOMB";
    }
    return "?";
}

static void ps_visit(const task_t *t, void *ctx)
{
    struct ps_ctx *c = (struct ps_ctx *)ctx;
    if (c->len >= c->cap)
        return;
    /* Tiny formatter (kprintf goes to console, not a buffer). */
    char tmp[80];
    int n = 0;

    /* pid (right-aligned width 3) */
    char pid_str[8] = {0};
    uint32_t pid = t->pid;
    int pi = 0;
    if (pid == 0) pid_str[pi++] = '0';
    else { char rev[8]; int r=0; while (pid) { rev[r++]='0'+(pid%10); pid/=10; }
           while (r--) pid_str[pi++] = rev[r]; }
    pid_str[pi] = 0;

    /* assemble */
    const char *name = t->name[0] ? t->name : "?";
    const char *st   = state_name(t->state);

    /* pid:3  name:10  state */
    int width_pid = 3;
    int len_pid = pi;
    for (int i = 0; i < width_pid - len_pid; i++) tmp[n++] = ' ';
    for (int i = 0; pid_str[i]; i++) tmp[n++] = pid_str[i];
    tmp[n++] = ' ';
    int wn = 0;
    for (int i = 0; name[i] && wn < 12; i++, wn++) tmp[n++] = name[i];
    while (wn < 12) { tmp[n++] = ' '; wn++; }
    for (int i = 0; st[i]; i++) tmp[n++] = st[i];
    tmp[n++] = '\n';

    for (int i = 0; i < n && c->len < c->cap; i++)
        c->buf[c->len++] = tmp[i];
}

static int32_t sys_ps(registers_t *r)
{
    char    *buf  = (char *)(uintptr_t)r->ebx;
    uint32_t size = r->ecx;
    if (!buf || size == 0)
        return -1;
    struct ps_ctx c = { buf, size, 0 };
    task_for_each(ps_visit, &c);
    return (int32_t)c.len;
}

static int32_t sys_meminfo(registers_t *r)
{
    uint32_t *used   = (uint32_t *)(uintptr_t)r->ebx;
    uint32_t *total  = (uint32_t *)(uintptr_t)r->ecx;
    uint32_t *frames = (uint32_t *)(uintptr_t)r->edx;
    if (used)   *used   = (uint32_t)heap_used();
    if (total)  *total  = (uint32_t)heap_total();
    if (frames) *frames = (uint32_t)pmm_free_frames();
    return 0;
}

/* ── Dispatcher table ───────────────────────────────────────────────── */

typedef int32_t (*syscall_fn_t)(registers_t *);

static const syscall_fn_t syscall_table[SYS_MAX] = {
    [SYS_EXIT]     = sys_exit,
    [SYS_WRITE]    = sys_write,
    [SYS_READ]     = sys_read,
    [SYS_GETPID]   = sys_getpid,
    [SYS_YIELD]    = sys_yield,
    [SYS_SLEEP_MS] = sys_sleep_ms,
    [SYS_UPTIME]   = sys_uptime,
    [SYS_PS]       = sys_ps,
    [SYS_MEMINFO]  = sys_meminfo,
};

void syscall_dispatch(registers_t *r)
{
    uint32_t n = r->eax;
    if (n >= SYS_MAX || !syscall_table[n]) {
        r->eax = (uint32_t)-1;
        return;
    }
    r->eax = (uint32_t)syscall_table[n](r);
}

void syscall_init(void)
{
    /* DPL3 = user-mode reachable; INT32 gate; present. */
    uint8_t flags = IDT_GATE_PRESENT | IDT_GATE_DPL3 | IDT_GATE_INT32;
    idt_set_gate(0x80, (uint32_t)(uintptr_t)syscall_entry,
                 GDT_KERNEL_CODE, flags);
    kprintf("[sys ] INT 0x80 gate installed, %u syscalls registered\n",
            (uint32_t)SYS_MAX);
}
