/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/syscalls/syscall.h
 *  Module:   syscalls
 *  Brief:    System call gate (INT 0x80) and number registry.
 *
 *  Calling convention (i386, register-based, Linux-style):
 *      eax = syscall number
 *      ebx = arg0
 *      ecx = arg1
 *      edx = arg2
 *      esi = arg3
 *      edi = arg4
 *      (return value in eax)
 *
 *  Numbers are HelixOS-native (not Linux-compatible). The shell calls
 *  these via the syscall() inline below; user-mode programs will use
 *  the same INT 0x80 gate once user mode is enabled.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_SYSCALLS_H
#define HELIX_SYSCALLS_H

#include <helix/kernel.h>

/* ── Syscall numbers ────────────────────────────────────────────────── */

#define SYS_EXIT      0
#define SYS_WRITE     1   /* (fd, buf, count) -> count */
#define SYS_READ      2   /* (fd, buf, count) -> count (blocks) */
#define SYS_GETPID    3   /* () -> pid */
#define SYS_YIELD     4   /* () -> 0 */
#define SYS_SLEEP_MS  5   /* (ms) -> 0 */
#define SYS_UPTIME    6   /* () -> uptime in ms */
#define SYS_PS        7   /* (buf, size) -> bytes written */
#define SYS_MEMINFO   8   /* (out_used, out_total, out_free_frames) -> 0 */

#define SYS_MAX       9

/* ── Standard fds ───────────────────────────────────────────────────── */

#define FD_STDIN      0
#define FD_STDOUT     1
#define FD_STDERR     2

/* ── Initialization ─────────────────────────────────────────────────── */

void syscall_init(void);

/* ── Inline wrappers (called from kernel code today, user code later) ── */

static inline int32_t syscall0(uint32_t n)
{
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(n) : "memory");
    return ret;
}

static inline int32_t syscall1(uint32_t n, uint32_t a)
{
    int32_t ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret) : "a"(n), "b"(a) : "memory");
    return ret;
}

static inline int32_t syscall3(uint32_t n, uint32_t a, uint32_t b, uint32_t c)
{
    int32_t ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return ret;
}

#endif /* HELIX_SYSCALLS_H */
