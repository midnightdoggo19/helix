/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/memory/vmm.c
 *  Module:   memory/vmm
 *  Brief:    Region allocator + page-fault handler.
 *
 *  Page-fault error code (CPU-pushed):
 *      bit 0   1 = protection violation, 0 = non-present page
 *      bit 1   1 = write, 0 = read
 *      bit 2   1 = user mode, 0 = supervisor
 *      bit 3   1 = reserved bit set in PDE/PTE
 *      bit 4   1 = instruction fetch (NX)
 * ─────────────────────────────────────────────────────────────────────
 */
#include "vmm.h"
#include "pmm.h"
#include "../arch/i386/isr.h"
#include "../lib/printf.h"

static inline uintptr_t read_cr2(void)
{
    uintptr_t v;
    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

static void page_fault_handler(registers_t *r)
{
    uintptr_t fault_addr = read_cr2();
    uint32_t  err = r->err_code;

    kprintf("\n[ PAGE FAULT ] virt=0x%08x  eip=0x%08x  err=0x%x\n",
            (uint32_t)fault_addr, r->eip, err);
    kprintf("  cause: %s, %s, %s%s%s\n",
            (err & 0x1) ? "protection" : "not-present",
            (err & 0x2) ? "write"      : "read",
            (err & 0x4) ? "user"       : "kernel",
            (err & 0x8) ? ", reserved-bit"  : "",
            (err & 0x10)? ", inst-fetch"    : "");

    kpanic("Unhandled page fault");
}

void vmm_init(void)
{
    isr_register(14, page_fault_handler);
    kprintf("[vmm ] page-fault handler installed (vector 14)\n");
}

bool vmm_alloc_region(uintptr_t virt, size_t pages, uint32_t flags)
{
    for (size_t i = 0; i < pages; i++) {
        uintptr_t phys = pmm_alloc_frame();
        if (phys == PMM_NO_FRAME) {
            /* Roll back. */
            for (size_t j = 0; j < i; j++) {
                uintptr_t p = paging_translate(virt + j * PAGE_SIZE);
                if (p)
                    pmm_free_frame(p);
                paging_unmap(virt + j * PAGE_SIZE);
            }
            return false;
        }
        paging_map(virt + i * PAGE_SIZE, phys, flags | PAGE_RW);
    }
    return true;
}

void vmm_free_region(uintptr_t virt, size_t pages)
{
    for (size_t i = 0; i < pages; i++) {
        uintptr_t p = paging_translate(virt + i * PAGE_SIZE);
        if (p)
            pmm_free_frame(p);
        paging_unmap(virt + i * PAGE_SIZE);
    }
}
