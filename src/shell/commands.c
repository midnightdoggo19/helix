/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/shell/commands.c
 *  Module:   shell/commands
 *  Brief:    Built-in shell command implementations.
 *
 *  Available commands:
 *      help         — list all commands
 *      version      — print kernel version banner
 *      clear        — clear the screen
 *      echo ARGS    — print ARGS, then newline
 *      uptime       — milliseconds since boot
 *      ps           — list all tasks
 *      mem          — heap + frame stats
 *      ls [PATH]    — list directory contents
 *      cat PATH     — print file contents
 *      touch PATH   — create empty file
 *      mkdir PATH   — create directory
 *      rm PATH      — remove file or empty directory
 *      write P TEXT — append TEXT (single word) to file P
 *      reboot       — triple-fault on purpose to restart the VM
 * ─────────────────────────────────────────────────────────────────────
 */
#include <helix/kernel.h>
#include <helix/version.h>

#include "../arch/i386/cpu.h"
#include "../drivers/vga/vga.h"
#include "../fs/vfs.h"
#include "../lib/printf.h"
#include "../lib/string.h"
#include "../syscalls/syscall.h"

/* ── Local utilities ────────────────────────────────────────────────── */

static void out(const char *s)
{
    syscall3(SYS_WRITE, FD_STDOUT, (uint32_t)(uintptr_t)s, strlen(s));
}

static void out_uint(uint32_t v)
{
    char buf[12];
    int  i = 0;
    if (v == 0) buf[i++] = '0';
    else {
        char rev[12]; int r = 0;
        while (v) { rev[r++] = (char)('0' + (v % 10)); v /= 10; }
        while (r--) buf[i++] = rev[r];
    }
    buf[i] = 0;
    out(buf);
}

/* ── Commands ───────────────────────────────────────────────────────── */

static int c_help(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    out("Built-in commands:\n");
    out("  help           Show this list\n");
    out("  version        Print kernel version\n");
    out("  clear          Clear the screen\n");
    out("  echo ARGS      Print ARGS\n");
    out("  uptime         Milliseconds since boot\n");
    out("  ps             List tasks\n");
    out("  mem            Memory statistics\n");
    out("  ls [PATH]      List directory (default /)\n");
    out("  cat PATH       Print file contents\n");
    out("  touch PATH     Create empty file\n");
    out("  mkdir PATH     Create directory\n");
    out("  rm PATH        Remove file/empty-dir\n");
    out("  write P TEXT   Append TEXT to file P\n");
    out("  reboot         Restart the machine\n");
    return 0;
}

static int c_version(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    out(helix_banner());
    out("\n");
    return 0;
}

static int c_clear(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    vga_clear();
    return 0;
}

static int c_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        out(argv[i]);
        if (i < argc - 1) out(" ");
    }
    out("\n");
    return 0;
}

static int c_uptime(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    uint32_t ms = (uint32_t)syscall0(SYS_UPTIME);
    out_uint(ms / 1000);
    out(".");
    uint32_t tenths = (ms % 1000) / 100;
    out_uint(tenths);
    out("s\n");
    return 0;
}

static int c_ps(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    char buf[512];
    int n = syscall3(SYS_PS, (uint32_t)(uintptr_t)buf, sizeof(buf), 0);
    if (n < 0) { out("ps failed\n"); return 0; }
    out("PID NAME         STATE\n");
    syscall3(SYS_WRITE, FD_STDOUT, (uint32_t)(uintptr_t)buf, (uint32_t)n);
    return 0;
}

static int c_mem(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    uint32_t used, total, frames;
    syscall3(SYS_MEMINFO,
             (uint32_t)(uintptr_t)&used,
             (uint32_t)(uintptr_t)&total,
             (uint32_t)(uintptr_t)&frames);
    out("Heap : "); out_uint(used / 1024);
    out(" / "); out_uint(total / 1024); out(" KiB used\n");
    out("Phys : "); out_uint(frames * 4 / 1024);
    out(" MiB free ("); out_uint(frames); out(" frames)\n");
    return 0;
}

static int c_ls(int argc, char **argv)
{
    const char *path = (argc >= 2) ? argv[1] : "/";
    vfs_node_t *n = vfs_lookup(path);
    if (!n) { out("ls: '"); out(path); out("': not found\n"); return 0; }
    if (n->kind != VFS_DIR) {
        out(n->name); out("\n");
        return 0;
    }
    for (vfs_node_t *c = n->children; c; c = c->next_sibling) {
        if (c->kind == VFS_DIR) {
            vga_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
            out(c->name);
            out("/");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        } else {
            out(c->name);
            out(" ("); out_uint((uint32_t)c->size); out(" B)");
        }
        out("\n");
    }
    return 0;
}

static int c_cat(int argc, char **argv)
{
    if (argc < 2) { out("cat: missing path\n"); return 0; }
    vfs_node_t *f = vfs_lookup(argv[1]);
    if (!f) { out("cat: '"); out(argv[1]); out("': not found\n"); return 0; }
    if (f->kind != VFS_FILE) { out("cat: not a regular file\n"); return 0; }
    syscall3(SYS_WRITE, FD_STDOUT,
             (uint32_t)(uintptr_t)f->data, (uint32_t)f->size);
    if (f->size > 0 && f->data[f->size - 1] != '\n')
        out("\n");
    return 0;
}

static int c_touch(int argc, char **argv)
{
    if (argc < 2) { out("touch: missing path\n"); return 0; }
    if (!vfs_touch(argv[1])) { out("touch: failed\n"); }
    return 0;
}

static int c_mkdir(int argc, char **argv)
{
    if (argc < 2) { out("mkdir: missing path\n"); return 0; }
    if (!vfs_mkdir(argv[1])) { out("mkdir: failed\n"); }
    return 0;
}

static int c_rm(int argc, char **argv)
{
    if (argc < 2) { out("rm: missing path\n"); return 0; }
    if (!vfs_remove(argv[1])) { out("rm: failed (not found or non-empty dir)\n"); }
    return 0;
}

static int c_write(int argc, char **argv)
{
    if (argc < 3) { out("write: usage: write PATH TEXT\n"); return 0; }
    vfs_node_t *f = vfs_lookup(argv[1]);
    if (!f) f = vfs_touch(argv[1]);
    if (!f || f->kind != VFS_FILE) { out("write: failed\n"); return 0; }
    vfs_write_append(f, argv[2], strlen(argv[2]));
    vfs_write_append(f, "\n", 1);
    return 0;
}

static int c_reboot(int argc, char **argv)
{
    UNUSED(argc); UNUSED(argv);
    out("Rebooting...\n");
    /* Cause a triple-fault: load a bogus IDT then trigger any interrupt.
     * QEMU resets the VM in response. */
    struct { uint16_t l; uint32_t b; } PACKED bogus = { 0, 0 };
    __asm__ volatile("lidt %0; int $0" : : "m"(bogus));
    cpu_hang();
    return 0;
}

/* ── Dispatch table ─────────────────────────────────────────────────── */

typedef int (*cmd_fn_t)(int, char **);
typedef struct { const char *name; cmd_fn_t fn; } cmd_t;

static const cmd_t commands[] = {
    { "help",    c_help    },
    { "version", c_version },
    { "clear",   c_clear   },
    { "echo",    c_echo    },
    { "uptime",  c_uptime  },
    { "ps",      c_ps      },
    { "mem",     c_mem     },
    { "ls",      c_ls      },
    { "cat",     c_cat     },
    { "touch",   c_touch   },
    { "mkdir",   c_mkdir   },
    { "rm",      c_rm      },
    { "write",   c_write   },
    { "reboot",  c_reboot  },
};

int cmd_dispatch(int argc, char **argv)
{
    if (argc < 1) return 0;
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].fn(argc, argv);
    }
    return -1;
}

void cmd_print_motd(void)
{
    vfs_node_t *motd = vfs_lookup("/etc/motd");
    if (motd && motd->kind == VFS_FILE && motd->size > 0)
        syscall3(SYS_WRITE, FD_STDOUT,
                 (uint32_t)(uintptr_t)motd->data, (uint32_t)motd->size);
}
