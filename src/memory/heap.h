/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/heap.h
 *  Module:   memory/heap
 *  Brief:    Kernel heap with first-fit free-list allocator.
 *
 *  Layout of a block:
 *      ┌──────────────────────────┬──────────────────────┐
 *      │      block_header_t      │   user data (size B) │
 *      └──────────────────────────┴──────────────────────┘
 *      ^                          ^
 *      header start               ptr returned to caller
 *
 *  Blocks live in a doubly-linked list ordered by address, mixing free
 *  and used. kmalloc walks the list first-fit; kfree coalesces with
 *  immediate neighbors when they are free.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_MEMORY_HEAP_H
#define HELIX_MEMORY_HEAP_H

#include <helix/kernel.h>

#define KHEAP_VIRT_START   0x00C00000U   /* 12 MiB */
#define KHEAP_INITIAL_SIZE (1 * 1024 * 1024)  /* 1 MiB */

/** Initialize the kernel heap; must be called after VMM is up. */
void heap_init(void);

/** Allocate @p size bytes. Returns NULL on failure. */
void *kmalloc(size_t size);

/** Release a previously kmalloc'd pointer. NULL is allowed. */
void  kfree(void *ptr);

/** kmalloc + memset(0). */
void *kcalloc(size_t n, size_t size);

/** Bytes currently in use (sum of allocated block sizes). */
size_t heap_used(void);

/** Total bytes the heap owns. */
size_t heap_total(void);

#endif /* HELIX_MEMORY_HEAP_H */
