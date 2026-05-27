/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/vmm.h
 *  Module:   memory/vmm
 *  Brief:    High-level virtual memory API + page-fault handler.
 *
 *  The VMM is intentionally thin: it presents a stable interface
 *  (map/unmap/alloc-region) so that callers don't need to know about
 *  PDE/PTE layout, and it owns the vector-14 (page fault) handler.
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_MEMORY_VMM_H
#define HELIX_MEMORY_VMM_H

#include "paging.h"

/** Register the page-fault handler. Call after isr_install(). */
void vmm_init(void);

/** Allocate @p pages physical frames and map them contiguously at @p virt. */
bool vmm_alloc_region(uintptr_t virt, size_t pages, uint32_t flags);

/** Reverse of vmm_alloc_region(). */
void vmm_free_region(uintptr_t virt, size_t pages);

#endif /* HELIX_MEMORY_VMM_H */
