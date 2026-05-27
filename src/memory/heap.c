/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/heap.c
 *  Module:   memory/heap
 *  Brief:    First-fit free-list kernel heap with split + coalesce.
 *
 *  Invariants maintained at all times:
 *      - The block list is sorted by ascending address.
 *      - Two adjacent free blocks are merged immediately on kfree.
 *      - block->magic must always equal HEAP_MAGIC.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "paging.h"

#include "../lib/printf.h"
#include "../lib/string.h"

#define HEAP_MAGIC      0xC0FFEE42U
#define HEAP_MIN_SPLIT  16  /* don't split if the remainder would be tiny */
#define HEAP_ALIGN      8

typedef struct block {
    uint32_t      magic;
    size_t        size;     /* size of payload, not including header */
    bool          free;
    struct block *next;
    struct block *prev;
} block_t;

static block_t *head;
static size_t   total_bytes;
static size_t   used_bytes;

/* ── Helpers ────────────────────────────────────────────────────────── */

static inline size_t align_up(size_t x, size_t a)
{
    return (x + (a - 1)) & ~(a - 1);
}

static inline block_t *header_of(void *user_ptr)
{
    return (block_t *)((uint8_t *)user_ptr - sizeof(block_t));
}

static inline void *payload_of(block_t *b)
{
    return (void *)((uint8_t *)b + sizeof(block_t));
}

static inline block_t *block_after(block_t *b)
{
    return (block_t *)((uint8_t *)payload_of(b) + b->size);
}

static void check(block_t *b, const char *where)
{
    if (b->magic != HEAP_MAGIC)
        kpanic("heap: corrupted block at %p (%s)", (void *)b, where);
}

static void try_coalesce(block_t *b)
{
    if (b->next && b->next->free && block_after(b) == b->next) {
        b->size += sizeof(block_t) + b->next->size;
        b->next  = b->next->next;
        if (b->next)
            b->next->prev = b;
    }
}

/* ── Init ───────────────────────────────────────────────────────────── */

void heap_init(void)
{
    size_t pages = KHEAP_INITIAL_SIZE / PAGE_SIZE;
    if (!vmm_alloc_region(KHEAP_VIRT_START, pages, PAGE_RW))
        kpanic("heap_init: vmm_alloc_region failed");

    head            = (block_t *)KHEAP_VIRT_START;
    head->magic     = HEAP_MAGIC;
    head->size      = KHEAP_INITIAL_SIZE - sizeof(block_t);
    head->free      = true;
    head->next      = NULL;
    head->prev      = NULL;

    total_bytes = KHEAP_INITIAL_SIZE;
    used_bytes  = 0;

    kprintf("[heap] %u KiB heap mapped at 0x%08x\n",
            (uint32_t)(KHEAP_INITIAL_SIZE / 1024),
            (uint32_t)KHEAP_VIRT_START);
}

/* ── kmalloc / kfree ────────────────────────────────────────────────── */

void *kmalloc(size_t size)
{
    if (size == 0)
        return NULL;
    size = align_up(size, HEAP_ALIGN);

    for (block_t *b = head; b; b = b->next) {
        check(b, "kmalloc walk");
        if (!b->free || b->size < size)
            continue;

        /* Split if there's enough room for a new free block after. */
        if (b->size >= size + sizeof(block_t) + HEAP_MIN_SPLIT) {
            block_t *rest = (block_t *)((uint8_t *)payload_of(b) + size);
            rest->magic = HEAP_MAGIC;
            rest->size  = b->size - size - sizeof(block_t);
            rest->free  = true;
            rest->next  = b->next;
            rest->prev  = b;
            if (b->next)
                b->next->prev = rest;
            b->next = rest;
            b->size = size;
        }

        b->free = false;
        used_bytes += b->size + sizeof(block_t);
        return payload_of(b);
    }
    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;
    block_t *b = header_of(ptr);
    check(b, "kfree");

    if (b->free)
        kpanic("heap: double free of %p", ptr);

    b->free = true;
    used_bytes -= b->size + sizeof(block_t);

    /* Coalesce forward then backward. */
    try_coalesce(b);
    if (b->prev && b->prev->free && block_after(b->prev) == b)
        try_coalesce(b->prev);
}

void *kcalloc(size_t n, size_t size)
{
    size_t total = n * size;
    void *p = kmalloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

size_t heap_used(void)  { return used_bytes;  }
size_t heap_total(void) { return total_bytes; }
