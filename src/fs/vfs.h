/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/fs/vfs.h
 *  Module:   fs/vfs
 *  Brief:    Minimal Virtual File System.
 *
 *  A node represents either a directory or a regular file. Directories
 *  hold a linked list of children. Regular files hold a heap-allocated
 *  byte buffer.
 *
 *  This is deliberately simple — no inodes, no permissions, no mounts
 *  table. One filesystem (RAMFS) is hard-mounted at /. Enough to give
 *  the shell something to read, write, and list.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_FS_VFS_H
#define HELIX_FS_VFS_H

#include <helix/kernel.h>

#define VFS_NAME_MAX  31

typedef enum {
    VFS_FILE,
    VFS_DIR,
} vfs_kind_t;

typedef struct vfs_node {
    char                name[VFS_NAME_MAX + 1];
    vfs_kind_t          kind;

    /* For files */
    uint8_t            *data;
    size_t              size;
    size_t              capacity;

    /* For directories */
    struct vfs_node    *children;
    struct vfs_node    *next_sibling;
    struct vfs_node    *parent;
} vfs_node_t;

/** Returns the root node (must be initialized by ramfs_init). */
vfs_node_t *vfs_root(void);

/** Resolve a path like "/foo/bar" → node, or NULL if not found. */
vfs_node_t *vfs_lookup(const char *path);

/** Read up to count bytes from offset. Returns bytes read (0..count). */
size_t vfs_read(vfs_node_t *f, size_t offset, void *buf, size_t count);

/** Append count bytes. Returns bytes written (0..count). */
size_t vfs_write_append(vfs_node_t *f, const void *buf, size_t count);

/** Create a regular file at @p path. Returns the new node or NULL. */
vfs_node_t *vfs_touch(const char *path);

/** Create a directory at @p path. Returns the new node or NULL. */
vfs_node_t *vfs_mkdir(const char *path);

/** Remove a file or empty directory. Returns true on success. */
bool vfs_remove(const char *path);

#endif /* HELIX_FS_VFS_H */
