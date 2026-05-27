/*
 * SPDX-License-Identifier: MIT
 * HelixOS linker script
 *
 * Places the kernel at the conventional 1 MiB physical address (where
 * GRUB will load it via Multiboot1) and exposes a kernel_end symbol
 * the PMM uses to know where free physical memory begins.
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* The multiboot header must appear in the first 8 KiB of the binary. */
    .multiboot ALIGN(4K) : {
        KEEP(*(.multiboot))
    }

    .text ALIGN(4K) : {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Exposed to C as `extern uint8_t kernel_end[];` */
    kernel_end = .;
}
