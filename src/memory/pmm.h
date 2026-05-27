/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/pmm.h
 *  Module:   memory/pmm
 *  Brief:    Physical Memory Manager — bitmap allocator.
 *
 *  Tracks every 4 KiB physical frame the system owns. One bit per
 *  frame: 0 = free, 1 = used. Memory map is derived from the
 *  Multiboot1 mmap entries handed to us by GRUB.
 *
 *  After pmm_init():
 *      - Frames containing the kernel image are marked used.
 *      - Frames containing the bitmap itself are marked used.
 *      - Frames in any non-AVAILABLE Multiboot region are marked used.
 *      - All other frames are free.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_MEMORY_PMM_H
#define HELIX_MEMORY_PMM_H

#include <helix/kernel.h>
#include "../boot/multiboot.h"

#define PAGE_SIZE      4096
#define PAGE_SHIFT     12
#define PAGE_MASK      (PAGE_SIZE - 1)

#define PMM_NO_FRAME   ((uintptr_t)0)

/** Initialize from the multiboot memory map. */
void pmm_init(multiboot_info_t *mb);

/** Allocate one 4 KiB frame. Returns physical address or PMM_NO_FRAME. */
uintptr_t pmm_alloc_frame(void);

/** Allocate @p count contiguous frames. Returns base phys addr or NO_FRAME. */
uintptr_t pmm_alloc_frames(size_t count);

/** Release a previously-allocated frame. */
void pmm_free_frame(uintptr_t phys);

/** Number of frames the PMM knows about. */
size_t pmm_total_frames(void);

/** Number of frames currently allocated. */
size_t pmm_used_frames(void);

/** Number of frames currently free. */
size_t pmm_free_frames(void);

#endif /* HELIX_MEMORY_PMM_H */
