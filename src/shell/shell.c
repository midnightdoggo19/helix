/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/shell/shell.c
 *  Module:   shell
 *  Brief:    Read a line, tokenize, dispatch to a built-in.
 *
 *  The shell uses ONLY syscalls (sys_write / sys_read) for I/O. No
 *  direct kprintf calls — the goal is to test the syscall layer the
 *  same way a future user-mode program would use it.
 *
 *  Commands live in commands.c so this file stays focused on the
 *  line editor + dispatch.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "shell.h"

#include "../drivers/keyboard/keyboard.h"
#include "../drivers/vga/vga.h"
#include "../lib/printf.h"
#include "../lib/string.h"
#include "../syscalls/syscall.h"

#define LINE_MAX  128
#define ARGV_MAX  8

extern int  cmd_dispatch(int argc, char **argv);
extern void cmd_print_motd(void);

/* ── Small I/O helpers built on top of syscalls ─────────────────────── */

static void sh_write(const char *s)
{
    syscall3(SYS_WRITE, FD_STDOUT, (uint32_t)(uintptr_t)s, strlen(s));
}

static void sh_writec(char c)
{
    syscall3(SYS_WRITE, FD_STDOUT, (uint32_t)(uintptr_t)&c, 1);
}

/* Render a colored prompt directly via VGA — color escapes are not
 * worth implementing for one prompt. */
static void sh_prompt(void)
{
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    sh_write("helix");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    sh_write(":/$ ");
}

/* ── Line editor: read one line into @p buf with basic echo + backspace ── */

static int sh_readline(char *buf, int cap)
{
    int len = 0;
    while (len < cap - 1) {
        char c = kbd_getchar();
        if (c == '\n') {
            sh_writec('\n');
            buf[len] = '\0';
            return len;
        }
        if (c == '\b') {
            if (len > 0) {
                len--;
                sh_writec('\b');
                sh_writec(' ');
                sh_writec('\b');
            }
            continue;
        }
        if (c >= 0x20 && c < 0x7F) {
            buf[len++] = c;
            sh_writec(c);
        }
    }
    buf[len] = '\0';
    return len;
}

/* ── Tokenizer: in-place split on whitespace ────────────────────────── */

static int sh_tokenize(char *line, char **argv, int cap)
{
    int argc = 0;
    char *p = line;
    while (*p && argc < cap) {
        while (*p == ' ' || *p == '\t')
            *p++ = '\0';
        if (*p == '\0')
            break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
    }
    return argc;
}

/* ── Main loop ──────────────────────────────────────────────────────── */

void shell_main(void)
{
    char  line[LINE_MAX];
    char *argv[ARGV_MAX];

    cmd_print_motd();
    sh_write("\n");

    for (;;) {
        sh_prompt();
        int n = sh_readline(line, sizeof(line));
        if (n == 0)
            continue;

        int argc = sh_tokenize(line, argv, ARGV_MAX);
        if (argc == 0)
            continue;

        int rc = cmd_dispatch(argc, argv);
        if (rc < 0) {
            sh_write("shell: '");
            sh_write(argv[0]);
            sh_write("': command not found\n");
        }
    }
}
