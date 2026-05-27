/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/boot/multiboot.h
 *  Module:   boot/multiboot
 *  Brief:    Multiboot1 specification structures.
 *
 *  GRUB hands us a pointer (in EBX at entry) to a multiboot_info_t
 *  that describes how the system was booted: memory map, command line,
 *  loaded modules, boot device, etc. This header declares the subset
 *  HelixOS actually consumes.
 *
 *  Reference: https://www.gnu.org/software/grub/manual/multiboot/
 * ─────────────────────────────────────────────────────────────────────
 */
#ifndef HELIX_BOOT_MULTIBOOT_H
#define HELIX_BOOT_MULTIBOOT_H

#include <helix/kernel.h>

#define MULTIBOOT_MAGIC               0x2BADB002

/* Flag bits in multiboot_info_t.flags */
#define MULTIBOOT_FLAG_MEM            (1U << 0)
#define MULTIBOOT_FLAG_BOOTDEV        (1U << 1)
#define MULTIBOOT_FLAG_CMDLINE        (1U << 2)
#define MULTIBOOT_FLAG_MODS           (1U << 3)
#define MULTIBOOT_FLAG_MMAP           (1U << 6)

/* Memory map entry types */
#define MULTIBOOT_MMAP_AVAILABLE      1
#define MULTIBOOT_MMAP_RESERVED       2
#define MULTIBOOT_MMAP_ACPI_RECLAIM   3
#define MULTIBOOT_MMAP_NVS            4
#define MULTIBOOT_MMAP_BADRAM         5

typedef struct PACKED multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;          /* KB below 1 MiB */
    uint32_t mem_upper;          /* KB above 1 MiB */
    uint32_t boot_device;
    uint32_t cmdline;            /* phys addr of command-line string */
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;          /* phys addr of memory-map array */
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot_info_t;

typedef struct PACKED multiboot_mmap_entry {
    uint32_t size;               /* size of this entry, NOT including 'size' */
    uint64_t addr;               /* base physical address */
    uint64_t len;                /* length in bytes */
    uint32_t type;               /* MULTIBOOT_MMAP_* */
} multiboot_mmap_entry_t;

#endif /* HELIX_BOOT_MULTIBOOT_H */
