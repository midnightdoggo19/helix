/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/paging.c
 *  Module:   memory/paging
 *  Brief:    Page directory/table manipulation; enable CR0.PG.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "paging.h"
#include "pmm.h"
#include "../lib/printf.h"
#include "../lib/string.h"

#define PD_INDEX(v)   (((v) >> 22) & 0x3FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x3FF)
#define ALIGN_PAGE(x) ALIGN_DOWN((x), PAGE_SIZE)

/** Kernel page directory — 4 KiB-aligned via __attribute__. */
static pde_t kernel_pd[1024] ALIGNED(PAGE_SIZE);

/* ── CR-register access ─────────────────────────────────────────────── */

static inline void load_cr3(uintptr_t pd_phys)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys) : "memory");
}

static inline uintptr_t read_cr3(void)
{
    uintptr_t v;
    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline void enable_paging(void)
{
    uintptr_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000U; /* PG bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

static inline void invlpg(uintptr_t v)
{
    __asm__ volatile("invlpg (%0)" : : "r"(v) : "memory");
}

void paging_flush_tlb(void)
{
    load_cr3(read_cr3());
}

/* ── Page-table helpers ─────────────────────────────────────────────── */

static pte_t *pt_for(uintptr_t virt, bool create, uint32_t flags)
{
    pde_t *pde = &kernel_pd[PD_INDEX(virt)];

    if (!(*pde & PAGE_PRESENT)) {
        if (!create)
            return NULL;

        uintptr_t pt_phys = pmm_alloc_frame();
        if (pt_phys == PMM_NO_FRAME)
            kpanic("paging: out of memory while allocating page table");

        memset((void *)pt_phys, 0, PAGE_SIZE);  /* identity map = OK now */
        *pde = pt_phys | (flags & PAGE_FLAGS_MASK) | PAGE_PRESENT | PAGE_RW;
    }
    return (pte_t *)(*pde & PAGE_ADDR_MASK);
}

/* ── Public API ─────────────────────────────────────────────────────── */

void paging_map(uintptr_t virt, uintptr_t phys, uint32_t flags)
{
    virt = ALIGN_PAGE(virt);
    phys = ALIGN_PAGE(phys);

    pte_t *pt = pt_for(virt, true, flags);
    pt[PT_INDEX(virt)] = phys | (flags & PAGE_FLAGS_MASK) | PAGE_PRESENT;
    invlpg(virt);
}

void paging_unmap(uintptr_t virt)
{
    virt = ALIGN_PAGE(virt);
    pte_t *pt = pt_for(virt, false, 0);
    if (!pt)
        return;
    pt[PT_INDEX(virt)] = 0;
    invlpg(virt);
}

uintptr_t paging_translate(uintptr_t virt)
{
    pte_t *pt = pt_for(virt, false, 0);
    if (!pt)
        return 0;
    pte_t entry = pt[PT_INDEX(virt)];
    if (!(entry & PAGE_PRESENT))
        return 0;
    return (entry & PAGE_ADDR_MASK) | (virt & PAGE_MASK);
}

/* ── Init ───────────────────────────────────────────────────────────── */

void paging_init(void)
{
    memset(kernel_pd, 0, sizeof(kernel_pd));

    /*
     * Identity-map the first 8 MiB: kernel image + bitmap + early heap.
     * 8 MiB = 2 PDEs worth (each PDE covers 4 MiB via 1024 PTEs).
     */
    for (uintptr_t addr = 0; addr < 8 * 1024 * 1024; addr += PAGE_SIZE) {
        paging_map(addr, addr, PAGE_RW);
    }

    load_cr3((uintptr_t)kernel_pd);
    enable_paging();

    kprintf("[paging] %u MiB identity-mapped, paging ON (CR3=0x%08x)\n",
            8U, (uint32_t)(uintptr_t)kernel_pd);
}
