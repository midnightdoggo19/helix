/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/fs/vfs.c
 *  Module:   fs/vfs
 *  Brief:    Path walker + read/write/create operating on RAMFS nodes.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "vfs.h"
#include "../lib/string.h"
#include "../memory/heap.h"

/* The root is allocated and installed by ramfs_init. */
static vfs_node_t *root_node;

vfs_node_t *vfs_root(void)
{
    return root_node;
}

void __vfs_set_root(vfs_node_t *n);  /* internal — used by ramfs_init */
void __vfs_set_root(vfs_node_t *n) { root_node = n; }

/* ── Path resolution ────────────────────────────────────────────────── */

/* Look up @p name (no slashes) among @p dir's children. */
static vfs_node_t *find_child(vfs_node_t *dir, const char *name, size_t len)
{
    if (!dir || dir->kind != VFS_DIR)
        return NULL;
    for (vfs_node_t *c = dir->children; c; c = c->next_sibling) {
        if (strlen(c->name) == len && strncmp(c->name, name, len) == 0)
            return c;
    }
    return NULL;
}

vfs_node_t *vfs_lookup(const char *path)
{
    if (!path || !root_node)
        return NULL;
    if (path[0] != '/')
        return NULL;
    if (path[1] == '\0')
        return root_node;

    vfs_node_t *cur = root_node;
    const char *p = path + 1;

    while (*p) {
        const char *slash = p;
        while (*slash && *slash != '/')
            slash++;
        size_t comp_len = (size_t)(slash - p);

        if (comp_len == 0) {
            /* trailing slash or // — skip */
            if (*slash) p = slash + 1;
            else break;
            continue;
        }

        cur = find_child(cur, p, comp_len);
        if (!cur)
            return NULL;

        if (*slash == '\0')
            break;
        if (cur->kind != VFS_DIR)
            return NULL; /* mid-path component must be a directory */
        p = slash + 1;
    }
    return cur;
}

/* Split a path into (parent_path, basename). The parent_path is written
 * into @p parent_buf (must be at least strlen(path)+1 bytes). Returns
 * a pointer to the basename within the original string, or NULL. */
static const char *split_parent(const char *path, char *parent_buf,
                                size_t buf_sz)
{
    size_t len = strlen(path);
    if (len == 0 || path[0] != '/')
        return NULL;

    /* Find the last slash. */
    ssize_t last_slash = -1;
    for (size_t i = 0; i < len; i++)
        if (path[i] == '/')
            last_slash = (ssize_t)i;
    if (last_slash < 0)
        return NULL;

    const char *base = path + last_slash + 1;
    if (*base == '\0')
        return NULL;

    if (last_slash == 0) {
        /* parent is root */
        if (buf_sz < 2) return NULL;
        parent_buf[0] = '/';
        parent_buf[1] = '\0';
    } else {
        if ((size_t)last_slash >= buf_sz) return NULL;
        for (ssize_t i = 0; i < last_slash; i++)
            parent_buf[i] = path[i];
        parent_buf[last_slash] = '\0';
    }
    return base;
}

/* ── File I/O ───────────────────────────────────────────────────────── */

size_t vfs_read(vfs_node_t *f, size_t offset, void *buf, size_t count)
{
    if (!f || f->kind != VFS_FILE || !buf)
        return 0;
    if (offset >= f->size)
        return 0;
    size_t avail = f->size - offset;
    size_t n = count < avail ? count : avail;
    memcpy(buf, f->data + offset, n);
    return n;
}

size_t vfs_write_append(vfs_node_t *f, const void *buf, size_t count)
{
    if (!f || f->kind != VFS_FILE || !buf || count == 0)
        return 0;

    size_t need = f->size + count;
    if (need > f->capacity) {
        size_t newcap = f->capacity ? f->capacity * 2 : 64;
        while (newcap < need)
            newcap *= 2;
        uint8_t *newbuf = (uint8_t *)kmalloc(newcap);
        if (!newbuf)
            return 0;
        if (f->data) {
            memcpy(newbuf, f->data, f->size);
            kfree(f->data);
        }
        f->data = newbuf;
        f->capacity = newcap;
    }
    memcpy(f->data + f->size, buf, count);
    f->size += count;
    return count;
}

/* ── Creation ───────────────────────────────────────────────────────── */

static vfs_node_t *create_node(vfs_node_t *parent, const char *name,
                               size_t name_len, vfs_kind_t kind)
{
    if (!parent || parent->kind != VFS_DIR || name_len > VFS_NAME_MAX)
        return NULL;
    if (find_child(parent, name, name_len))
        return NULL;

    vfs_node_t *n = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!n) return NULL;
    memset(n, 0, sizeof(*n));
    for (size_t i = 0; i < name_len; i++)
        n->name[i] = name[i];
    n->name[name_len] = '\0';
    n->kind = kind;
    n->parent = parent;

    /* prepend to children list */
    n->next_sibling = parent->children;
    parent->children = n;
    return n;
}

vfs_node_t *vfs_touch(const char *path)
{
    char parent_buf[128];
    const char *base = split_parent(path, parent_buf, sizeof(parent_buf));
    if (!base) return NULL;
    vfs_node_t *parent = vfs_lookup(parent_buf);
    return create_node(parent, base, strlen(base), VFS_FILE);
}

vfs_node_t *vfs_mkdir(const char *path)
{
    char parent_buf[128];
    const char *base = split_parent(path, parent_buf, sizeof(parent_buf));
    if (!base) return NULL;
    vfs_node_t *parent = vfs_lookup(parent_buf);
    return create_node(parent, base, strlen(base), VFS_DIR);
}

bool vfs_remove(const char *path)
{
    vfs_node_t *n = vfs_lookup(path);
    if (!n || n == root_node)
        return false;
    if (n->kind == VFS_DIR && n->children)
        return false; /* non-empty */

    vfs_node_t *p = n->parent;
    if (!p) return false;

    /* unlink */
    vfs_node_t **pp = &p->children;
    while (*pp && *pp != n)
        pp = &(*pp)->next_sibling;
    if (!*pp) return false;
    *pp = n->next_sibling;

    if (n->kind == VFS_FILE && n->data)
        kfree(n->data);
    kfree(n);
    return true;
}
