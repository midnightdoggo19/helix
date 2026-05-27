/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/fs/ramfs.c
 *  Module:   fs/ramfs
 *  Brief:    Populate the VFS with an initial directory tree.
 *
 *  Layout after init:
 *      /
 *      ├── etc/
 *      │   └── motd            "Welcome to HelixOS ..."
 *      ├── tmp/                (empty directory)
 *      └── README              "HelixOS is a 32-bit ..."
 * ─────────────────────────────────────────────────────────────────────
 */
#include "ramfs.h"
#include "vfs.h"

#include "../lib/printf.h"
#include "../lib/string.h"
#include "../memory/heap.h"

extern void __vfs_set_root(struct vfs_node *n);  /* internal in vfs.c */

static void write_str(vfs_node_t *f, const char *s)
{
    vfs_write_append(f, s, strlen(s));
}

void ramfs_init(void)
{
    /* Build root by hand (no parent path to look up). */
    vfs_node_t *root = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!root)
        return;
    memset(root, 0, sizeof(*root));
    root->name[0] = '/';
    root->name[1] = '\0';
    root->kind = VFS_DIR;

    __vfs_set_root(root);

    /* Seed contents via the normal VFS API. */
    vfs_mkdir("/etc");
    vfs_mkdir("/tmp");

    vfs_node_t *motd = vfs_touch("/etc/motd");
    if (motd) {
        write_str(motd,
            "Welcome to HelixOS -- 32-bit educational kernel\n"
            "Type 'help' to list shell commands.\n");
    }

    vfs_node_t *readme = vfs_touch("/README");
    if (readme) {
        write_str(readme,
            "HelixOS is a 32-bit x86 kernel built from scratch in C\n"
            "and NASM. Subsystems include paging, IRQ-driven drivers,\n"
            "a Round-Robin scheduler, a syscall layer (INT 0x80), and\n"
            "this in-memory file system.\n"
            "\n"
            "Source: <your repo URL here>\n");
    }

    kprintf("[ramfs] mounted at / with %u entries seeded\n", 4U);
}
