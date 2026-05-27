/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/paging.h
 *  Module:   memory/paging
 *  Brief:    32-bit (non-PAE) 4 KiB paging structures.
 *
 *  Virtual address layout:
 *      31              22 21              12 11             0
 *      [  PD index  ][   PT index   ][  byte offset  ]
 *
 *  PDE/PTE flag bits (low 12 bits):
 *      bit 0   Present
 *      bit 1   Read/Write
 *      bit 2   User (ring 3 accessible)
 *      bit 3   PWT (write-through)
 *      bit 4   PCD (cache disable)
 *      bit 5   Accessed
 *      bit 6   Dirty (PTE only)
 *      bit 7   PS (PDE only; 1 = 4 MiB page)
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_MEMORY_PAGING_H
#define HELIX_MEMORY_PAGING_H

#include <helix/kernel.h>

#define PAGE_PRESENT    (1U << 0)
#define PAGE_RW         (1U << 1)
#define PAGE_USER       (1U << 2)
#define PAGE_PWT        (1U << 3)
#define PAGE_PCD        (1U << 4)
#define PAGE_ACCESSED   (1U << 5)
#define PAGE_DIRTY      (1U << 6)

#define PAGE_FLAGS_MASK 0xFFFU
#define PAGE_ADDR_MASK  0xFFFFF000U

typedef uint32_t pde_t;
typedef uint32_t pte_t;

/** Build the kernel page directory, identity-map low memory, enable paging. */
void paging_init(void);

/**
 * Map @p virt → @p phys with the given flags. PAGE_PRESENT is OR'd in.
 * Allocates a new page table via the PMM if needed.
 */
void paging_map(uintptr_t virt, uintptr_t phys, uint32_t flags);

/** Remove the mapping for @p virt. Safe to call on unmapped addresses. */
void paging_unmap(uintptr_t virt);

/** Translate @p virt to its physical address, or 0 if unmapped. */
uintptr_t paging_translate(uintptr_t virt);

/** Reload CR3 to flush the TLB. */
void paging_flush_tlb(void);

#endif /* HELIX_MEMORY_PAGING_H */
