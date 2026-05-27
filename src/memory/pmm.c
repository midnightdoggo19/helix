/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/pmm.c
 *  Module:   memory/pmm
 *  Brief:    Bitmap physical-frame allocator.
 *
 *  Index/frame relationship:
 *      frame_index  = phys_addr / PAGE_SIZE
 *      byte         = frame_index / 8
 *      bit          = frame_index % 8
 *
 *  Allocation strategy: first-fit linear scan from `next_hint`. After
 *  free, hint is dragged backwards so we don't scan the whole bitmap
 *  on every alloc once the heap stabilizes.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "pmm.h"
#include "../lib/printf.h"
#include "../lib/string.h"

/* Provided by the linker script. */
extern uint8_t kernel_end[];

static uint8_t  *bitmap;
static size_t    bitmap_bytes;
static size_t    total_frames;
static size_t    used_frames;
static size_t    next_hint;

/* ── Bitmap helpers ─────────────────────────────────────────────────── */

static inline bool bm_test(size_t i)
{
    return (bitmap[i >> 3] >> (i & 7)) & 1u;
}

static inline void bm_set(size_t i)
{
    bitmap[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static inline void bm_clear(size_t i)
{
    bitmap[i >> 3] &= (uint8_t)~(1u << (i & 7));
}

static void mark_region_used(uintptr_t base, size_t len)
{
    uintptr_t first = base >> PAGE_SHIFT;
    uintptr_t last  = (base + len - 1) >> PAGE_SHIFT;
    for (uintptr_t i = first; i <= last && i < total_frames; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            used_frames++;
        }
    }
}

static void mark_region_free(uintptr_t base, size_t len)
{
    /* Trim to page boundaries: don't free a partial frame. */
    uintptr_t start = ALIGN_UP(base, PAGE_SIZE);
    uintptr_t end   = ALIGN_DOWN(base + len, PAGE_SIZE);
    if (end <= start)
        return;

    uintptr_t first = start >> PAGE_SHIFT;
    uintptr_t last  = (end - 1) >> PAGE_SHIFT;
    for (uintptr_t i = first; i <= last && i < total_frames; i++) {
        if (bm_test(i)) {
            bm_clear(i);
            if (used_frames > 0)
                used_frames--;
        }
    }
}

/* ── Init ───────────────────────────────────────────────────────────── */

void pmm_init(multiboot_info_t *mb)
{
    if (!mb || !(mb->flags & MULTIBOOT_FLAG_MMAP))
        kpanic("pmm_init: multiboot mmap missing");

    /* Determine highest physical address from mmap. */
    uintptr_t highest_addr = 0;
    multiboot_mmap_entry_t *e = (multiboot_mmap_entry_t *)(uintptr_t)mb->mmap_addr;
    uintptr_t mmap_end = (uintptr_t)mb->mmap_addr + mb->mmap_length;

    while ((uintptr_t)e < mmap_end) {
        uintptr_t top = (uintptr_t)(e->addr + e->len);
        if (top > highest_addr)
            highest_addr = top;
        e = (multiboot_mmap_entry_t *)((uintptr_t)e + e->size + 4);
    }

    total_frames = highest_addr >> PAGE_SHIFT;
    bitmap_bytes = ALIGN_UP(total_frames, 8) / 8;

    /* Place bitmap immediately after the kernel image. */
    bitmap = (uint8_t *)ALIGN_UP((uintptr_t)kernel_end, PAGE_SIZE);

    /* Default: everything used (safer than the other way around). */
    memset(bitmap, 0xFF, bitmap_bytes);
    used_frames = total_frames;

    /* Free every AVAILABLE region. */
    e = (multiboot_mmap_entry_t *)(uintptr_t)mb->mmap_addr;
    while ((uintptr_t)e < mmap_end) {
        if (e->type == MULTIBOOT_MMAP_AVAILABLE)
            mark_region_free((uintptr_t)e->addr, (size_t)e->len);
        e = (multiboot_mmap_entry_t *)((uintptr_t)e + e->size + 4);
    }

    /* Reserve the first 1 MiB (legacy BIOS / VGA / IVT). */
    mark_region_used(0, 0x100000);

    /* Reserve the kernel image. */
    mark_region_used(0x100000,
                     (uintptr_t)kernel_end - 0x100000);

    /* Reserve the bitmap itself. */
    mark_region_used((uintptr_t)bitmap, bitmap_bytes);

    /* Start linear search from just above the reserved region. */
    next_hint = ((uintptr_t)bitmap + bitmap_bytes) >> PAGE_SHIFT;

    kprintf("[pmm] %u total frames, %u used, %u free (%u KiB usable)\n",
            (uint32_t)total_frames,
            (uint32_t)used_frames,
            (uint32_t)pmm_free_frames(),
            (uint32_t)(pmm_free_frames() * 4));
    kprintf("[pmm] bitmap at 0x%08x (%u bytes)\n",
            (uint32_t)(uintptr_t)bitmap,
            (uint32_t)bitmap_bytes);
}

/* ── Allocation ─────────────────────────────────────────────────────── */

uintptr_t pmm_alloc_frame(void)
{
    for (size_t i = next_hint; i < total_frames; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            used_frames++;
            next_hint = i + 1;
            return (uintptr_t)i << PAGE_SHIFT;
        }
    }
    /* Wrap around once. */
    for (size_t i = 0; i < next_hint; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            used_frames++;
            next_hint = i + 1;
            return (uintptr_t)i << PAGE_SHIFT;
        }
    }
    return PMM_NO_FRAME;
}

uintptr_t pmm_alloc_frames(size_t count)
{
    if (count == 0)
        return PMM_NO_FRAME;
    if (count == 1)
        return pmm_alloc_frame();

    /* Linear scan for a run of `count` consecutive zero bits. */
    size_t run = 0;
    size_t start = 0;
    for (size_t i = 0; i < total_frames; i++) {
        if (!bm_test(i)) {
            if (run == 0)
                start = i;
            run++;
            if (run == count) {
                for (size_t j = 0; j < count; j++)
                    bm_set(start + j);
                used_frames += count;
                return (uintptr_t)start << PAGE_SHIFT;
            }
        } else {
            run = 0;
        }
    }
    return PMM_NO_FRAME;
}

void pmm_free_frame(uintptr_t phys)
{
    size_t idx = phys >> PAGE_SHIFT;
    if (idx >= total_frames)
        return;
    if (!bm_test(idx))
        return; /* double free — silently ignore */
    bm_clear(idx);
    if (used_frames > 0)
        used_frames--;
    if (idx < next_hint)
        next_hint = idx;
}

size_t pmm_total_frames(void) { return total_frames; }
size_t pmm_used_frames(void)  { return used_frames;  }
size_t pmm_free_frames(void)  { return total_frames - used_frames; }
