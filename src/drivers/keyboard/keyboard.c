/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 HelixOS Authors
 *
 * ─────────────────────────────────────────────────────────────────────
 *  File:     src/drivers/keyboard/keyboard.c
 *  Module:   drivers/keyboard
 *  Brief:    IRQ-1 keyboard driver implementation.
 *
 *  Modifier handling:
 *      Shift  — tracked across press/release (either side)
 *      Caps   — toggled on press
 *      Letters: shift XOR caps = uppercase
 *      Other keys: shift only (caps has no effect)
 *
 *  Ring buffer is single-producer (IRQ handler) / single-consumer
 *  (kbd_getchar). head and tail are 32-bit volatile so non-atomic
 *  reads are safe for our single-CPU target.
 * ─────────────────────────────────────────────────────────────────────
 */
#include "keyboard.h"
#include "scancodes.h"

#include "../../arch/i386/cpu.h"
#include "../../arch/i386/io.h"
#include "../../arch/i386/irq.h"
#include "../../arch/i386/isr.h"
#include "../../arch/i386/pic.h"
#include "../../lib/printf.h"

#define KBD_DATA      0x60
#define KBD_STATUS    0x64

#define KBD_BUF_SIZE  128

static volatile char  ring[KBD_BUF_SIZE];
static volatile uint32_t head;
static volatile uint32_t tail;

static volatile bool shift_down;
static volatile bool caps_locked;

/* ── Producer (IRQ context) ─────────────────────────────────────────── */

static void buffer_push(char c)
{
    uint32_t next = (head + 1) % KBD_BUF_SIZE;
    if (next == tail)
        return;             /* buffer full — drop */
    ring[head] = c;
    head = next;
}

static void kbd_handler(registers_t *regs)
{
    UNUSED(regs);

    uint8_t code = inb(KBD_DATA);
    bool release = code & SC_RELEASE_MASK;
    uint8_t make = code & 0x7F;

    /* Modifier keys: track and return. */
    if (make == SC_LSHIFT || make == SC_RSHIFT) {
        shift_down = !release;
        return;
    }
    if (make == SC_CAPSLOCK) {
        if (!release)
            caps_locked = !caps_locked;
        return;
    }
    /* (LCTRL, LALT, function keys: ignored for now) */

    if (release)
        return;

    if (make >= sizeof(scancode_to_ascii))
        return;

    char ch  = scancode_to_ascii[make];
    char chS = scancode_to_ascii_shift[make];
    if (ch == 0)
        return;

    /* Letters honor caps; symbols honor shift only. */
    bool is_letter = (ch >= 'a' && ch <= 'z');
    bool upper = is_letter
        ? (shift_down ^ caps_locked)
        : shift_down;

    buffer_push(upper ? chS : ch);
}

/* ── Public API ─────────────────────────────────────────────────────── */

bool kbd_has_input(void)
{
    return head != tail;
}

int kbd_poll(char *c)
{
    if (head == tail)
        return 0;
    *c = ring[tail];
    tail = (tail + 1) % KBD_BUF_SIZE;
    return 1;
}

char kbd_getchar(void)
{
    char c;
    while (!kbd_poll(&c))
        cpu_halt();
    return c;
}

void kbd_init(void)
{
    head = tail = 0;
    shift_down = caps_locked = false;

    /* Drain any stale scancode left over from boot. */
    while (inb(KBD_STATUS) & 1)
        (void)inb(KBD_DATA);

    irq_register(1, kbd_handler);
    pic_unmask(1);

    kprintf("[kbd ] PS/2 keyboard ready (IRQ 1 unmasked)\n");
}
