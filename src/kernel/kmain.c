/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/kernel/kmain.c
 *  Module:   kernel/main
 *  Brief:    Kernel entry. Final init order (Phase 3f).
 *
 *  Init order:
 *      1.  serial    debug log channel
 *      2.  vga       primary console
 *      3.  gdt       replace GRUB's transient GDT
 *      4.  idt       empty 256-entry IDT
 *      5.  isr       CPU exception vectors (0..31)
 *      6.  irq       hardware IRQ vectors (32..47)
 *      7.  pic       remap 8259 PIC
 *      8.  pmm       physical memory bitmap
 *      9.  paging    page directory + identity map, CR0.PG = 1
 *     10.  vmm       page-fault handler + region allocator
 *     11.  heap      kmalloc / kfree
 *     12.  pit       100 Hz timer (drives scheduler_tick)
 *     13.  kbd       PS/2 keyboard
 *     14.  task      init task; scheduler online
 *     15.  syscall   INT 0x80 gate
 *     16.  ramfs     /, /etc/motd, /README, /tmp
 *     17.  sti       safe to enable interrupts
 *     18.  spawn shell task
 *     19.  init task idles
 * ─────────────────────────────────────────────────────────────────────
 */
#include <helix/kernel.h>
#include <helix/version.h>

#include "../arch/i386/cpu.h"
#include "../arch/i386/gdt.h"
#include "../arch/i386/idt.h"
#include "../arch/i386/irq.h"
#include "../arch/i386/isr.h"
#include "../arch/i386/pic.h"
#include "../boot/multiboot.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/serial/serial.h"
#include "../drivers/timer/pit.h"
#include "../drivers/vga/vga.h"
#include "../fs/ramfs.h"
#include "../memory/heap.h"
#include "../memory/paging.h"
#include "../memory/pmm.h"
#include "../memory/vmm.h"
#include "../process/scheduler.h"
#include "../process/task.h"
#include "../shell/shell.h"
#include "../syscalls/syscall.h"
#include "../lib/printf.h"

static void print_banner(void);
static void log_stage(const char *name);

NORETURN void kmain(uint32_t magic, multiboot_info_t *mb_info)
{
    serial_init();
    serial_puts("\n[boot] serial initialized\n");
    vga_init();
    print_banner();
    kputs("\n");

    log_stage("GDT      ");  gdt_init();
    log_stage("IDT      ");  idt_init();
    log_stage("ISRs     ");  isr_install();
    log_stage("IRQs     ");  irq_install();
    log_stage("PIC      ");  pic_init();

    kputs("\n");
    pmm_init(mb_info);
    paging_init();
    vmm_init();
    heap_init();

    log_stage("interrupts");
    cpu_sti();

    if (magic != MULTIBOOT_MAGIC)
        kpanic("Bad multiboot magic: 0x%08x", magic);

    kputs("\n");
    pit_init();
    kbd_init();
    task_init();
    syscall_init();
    ramfs_init();

    kputs("\n");
    vga_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    kputs("[ READY ] HelixOS Phase 3f -- spawning shell\n\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    task_create("shell", shell_main);

    /* Init task becomes the idle loop. */
    for (;;)
        cpu_halt();
}

static void log_stage(const char *name)
{
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    kputs(" [");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kputs("ok");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    kputs("] ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("%s initialized\n", name);
}

static void print_banner(void)
{
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kputs(
        "  _   _      _ _      ___  ____\n"
        " | | | | ___| (_)_  _/ _ \\/ ___|\n"
        " | |_| |/ _ \\ | \\ \\/ / | | \\___ \\\n"
        " |  _  |  __/ | |>  <| |_| |___) |\n"
        " |_| |_|\\___|_|_/_/\\_\\\\___/|____/\n"
    );
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    kprintf(" %s\n", helix_banner());
    kputs(" ----------------------------------------\n");
}
