# Driver API Contracts

The driver layer sits between hardware (port I/O, MMIO, IRQs) and the rest of the kernel. This doc defines what every driver is expected to provide and how it integrates.

## Driver life cycle

Every driver in HelixOS follows the same pattern:

```c
void <driver>_init(void) {
    /* 1. Program the hardware to a known state */
    outb(...);
    /* 2. (IRQ-driven only) register the C handler */
    irq_register(N, my_handler);
    /* 3. (IRQ-driven only) unmask the PIC line */
    pic_unmask(N);
    /* 4. Log readiness */
    kprintf("[<name>] ready\n");
}
```

That's it. There's no `<driver>_shutdown` in v0.1 — the kernel never tears down. There's no driver model with vtables, no probing, no hotplug. Simple, finite set, known at compile time.

## The four built-in drivers

| Driver | IRQ | Interface | LoC |
|---|---|---|---|
| **VGA text mode** | (polled; no IRQ) | byte → screen | 130 |
| **Serial (COM1)** | (polled write; no IRQ) | byte → serial port | 65 |
| **PIT (timer)** | IRQ 0 | provides tick + sleep | 60 |
| **PS/2 keyboard** | IRQ 1 | ASCII ring buffer | 100 |

## VGA driver

**Hardware**: legacy 80×25 text-mode framebuffer at physical 0xB8000. Each cell is 2 bytes: ASCII codepoint + attribute (`bg<<4 | fg`).

**API**:

```c
void vga_init(void);
void vga_clear(void);
void vga_putc(char c);     // handles \n \r \b \t + scroll
void vga_puts(const char *s);
void vga_set_color(vga_color_t fg, vga_color_t bg);
void vga_set_cursor(uint8_t x, uint8_t y);
```

**Special characters**:

| Input | Effect |
|---|---|
| `\n` | Move to start of next line; scroll if at row 24 |
| `\r` | Move cursor to column 0 |
| `\b` | Backspace: move left, blank the new cursor cell |
| `\t` | Advance to next 4-column boundary |

**Scroll**: shift all rows up by one, blank the new bottom row.

**Hardware cursor**: every `vga_putc` writes the cursor position to CRTC registers 0x0E/0x0F via ports 0x3D4/0x3D5.

## Serial driver

**Hardware**: 16550 UART at COM1 (base I/O 0x3F8). Used for debug output that survives even when VGA is broken. Running QEMU with `-serial stdio` redirects to the host terminal.

**API**:

```c
void serial_init(void);              // 38400 baud, 8N1, FIFOs on
void serial_putc(char c);            // busy-waits for TX
void serial_puts(const char *s);
```

**LF → CRLF translation**: `serial_putc('\n')` sends `\r\n` so dumb terminals show line breaks correctly.

**No reads in v0.1.** Input from serial would require the `irq_register(4, ...)` path and a ring buffer; we don't need it because the keyboard exists.

## PIT driver

**Hardware**: 8253/8254 Programmable Interval Timer. Channel 0 is wired to IRQ 0. Base clock is 1.193182 MHz; programming a divisor produces `1193182 / divisor` Hz.

**Configuration we use**: 100 Hz → 10 ms per tick → divisor 11931.

**API**:

```c
void     pit_init(void);
uint32_t pit_ticks(void);            // monotonic tick counter
uint32_t pit_uptime_ms(void);        // ticks * 10
void     pit_sleep_ms(uint32_t ms);  // halts until deadline
```

**IRQ handler responsibility**:

```c
static void pit_handler(registers_t *r) {
    ticks++;
    scheduler_tick();    // drives preemption
}
```

This is the engine of the entire OS. Every preemption, every wake-from-sleep, every timeout goes through this 5-line function.

## Keyboard driver

**Hardware**: PS/2 keyboard controller. Data port 0x60, status port 0x64. Set 1 scancodes (the legacy default). IRQ 1 fires for both press and release.

**Modifier tracking**: shift (either side), caps lock. Letters honor `shift XOR caps`; non-letter keys honor `shift` only.

**API**:

```c
void kbd_init(void);
char kbd_getchar(void);              // blocking
int  kbd_poll(char *out);            // 1 if char read, 0 if buffer empty
bool kbd_has_input(void);
```

**Internals**:

- 128-byte single-producer/single-consumer ring buffer
- IRQ handler is the producer; user code is the consumer
- `head`/`tail` are `volatile uint32_t`; safe on single CPU without atomics

**Scancode tables**: [`scancodes.h`](../../src/drivers/keyboard/scancodes.h) — two 60-byte arrays, one for unshifted and one for shifted output. Index = scancode (make code 0x00..0x39).

**Not yet supported**:

- Function keys (F1-F12)
- Arrow keys (require recognizing the 0xE0 prefix)
- Ctrl combinations
- Non-US layouts (would just require swapping in different tables)
- Caps Lock LED feedback (would require sending back to the controller)

## Adding a new driver

The minimum recipe:

1. Create `src/drivers/<name>/<name>.{c,h}`.
2. Public API gets:
   - `<name>_init(void)` — sets hardware to known state, registers IRQ if any
   - At least one accessor (`<name>_read`, `<name>_send`, ...)
3. Include `<arch/i386/io.h>` for port I/O. If IRQ-driven, include `<arch/i386/irq.h>` and `<arch/i386/pic.h>`.
4. Call `<name>_init()` from `kmain.c` at the appropriate point in init order.
5. If allocating, do it after `heap_init` runs.
6. Document the IRQ line you claimed in this file.

### A minimal IRQ-driven driver template

```c
#include "../../arch/i386/io.h"
#include "../../arch/i386/irq.h"
#include "../../arch/i386/isr.h"
#include "../../arch/i386/pic.h"
#include "../../lib/printf.h"

#define MYDEV_IRQ   N

static void my_handler(registers_t *r) {
    UNUSED(r);
    uint8_t v = inb(MYDEV_PORT);
    /* ... handle ... */
}

void mydev_init(void) {
    /* Configure the hardware */
    outb(MYDEV_CTRL, 0x...);

    irq_register(MYDEV_IRQ, my_handler);
    pic_unmask(MYDEV_IRQ);
    kprintf("[mydev] ready (IRQ %u)\n", MYDEV_IRQ);
}
```

### IRQ map (claimed lines)

| IRQ | Driver |
|---|---|
| 0 | PIT (`drivers/timer/pit.c`) |
| 1 | PS/2 keyboard (`drivers/keyboard/keyboard.c`) |
| 2..15 | (available) |

When adding a driver, update this table.
