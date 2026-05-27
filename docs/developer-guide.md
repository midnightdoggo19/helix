# Developer Guide

How to work on HelixOS day-to-day.

## Source organization

See [`docs/modules.md`](modules.md) for a per-folder rationale. The 30-second version:

- `src/arch/i386/` — anything x86-specific
- `src/memory/` — PMM, paging, VMM, heap (in that dependency order)
- `src/drivers/` — one folder per device
- `src/process/` — task, scheduler, sync
- `src/syscalls/` — INT 0x80 gate
- `src/fs/` — VFS + RAMFS
- `src/shell/` — interactive shell
- `src/lib/` — freestanding libc subset
- `include/helix/` — kernel-wide headers

Each subsystem has a `<thing>.h` + `<thing>.c`. Asm files live next to the C that calls them (`gdt.c` ↔ `gdt_flush.asm`).

## Coding standards

- **C11**, freestanding (`-ffreestanding -nostdlib -fno-builtin`)
- **`-Wall -Wextra -Werror`** (no warnings ever; CI rejects them)
- **`clang-format`** enforces brace style — run `make format` before committing
- **4-space indent**, no tabs (except in `Makefile`)
- **Tabs in NASM files** for the body — 4 columns
- **`snake_case`** for functions and variables
- **`UPPER_SNAKE`** for macros and enum members
- **`PascalCase`** is not used
- **Doxygen-style** comments for public functions:

```c
/**
 * Brief one-line summary.
 *
 * Longer description if needed.
 *
 * @param x  What x is
 * @return   What's returned (and what NULL/-1 means)
 */
void *frob(int x);
```

## Adding a new module

Say you want to add a small caching layer in front of the PMM.

1. **Pick the right directory.** A PMM cache is memory infrastructure → `src/memory/pmm_cache.{c,h}`.
2. **Write the header first.** Define the public API in 5-15 lines.
3. **Write the C body.** Keep public functions small; internal helpers `static`.
4. **Update `kmain.c`** if your module needs initialization.
5. **Run `make`.** The Makefile auto-discovers new `.c` files under `src/`.
6. **Run `make lint`.** Clean cppcheck output.
7. **Run `make run`** and verify nothing regressed.
8. **Document** in `docs/api/kernel-api.md` (one row in the relevant table).
9. **Write an ADR** if your module makes a *decision* worth recording. Most don't.

## Adding a new driver

See [`docs/api/drivers.md`](api/drivers.md#adding-a-new-driver) for the template. Briefly:

- Folder `src/drivers/<name>/`
- File `<name>.c`, `<name>.h`
- Init function calls `irq_register` + `pic_unmask` if IRQ-driven
- Wire into `kmain.c`
- Update the IRQ map at the bottom of `docs/api/drivers.md`

## Adding a new syscall

See [`docs/api/syscalls.md`](api/syscalls.md#adding-a-new-syscall). Briefly:

- Pick a number in `syscall.h`, bump `SYS_MAX`
- Write `static int32_t sys_X(registers_t *r)` in `syscall.c`
- Add to `syscall_table[]`
- Document in `syscalls.md`

## Debugging workflow

### Step 1 — Watch the serial log

```bash
make iso
qemu-system-i386 -cdrom build/helix.iso -m 128M -serial stdio
```

Every `kprintf` shows up. If you suspect a problem, add `kprintf("[debug] reached X\n")` and rebuild. (Yes, real kernel devs do this too.)

### Step 2 — GDB inside QEMU

```bash
make debug              # boots QEMU paused, GDB server on :1234
# in another terminal:
i686-elf-gdb build/helix.bin \
    -ex "target remote :1234" \
    -ex "break kmain" \
    -ex "continue"
```

Useful GDB commands:

| Command | Effect |
|---|---|
| `break <fn>` | Break at function entry |
| `step` / `next` | Single-step (into / over) |
| `info registers` | Dump CPU state |
| `info threads` | List task PCBs (if you've added GDB scripts for them) |
| `x/16wx 0x100000` | Hexdump 16 words at address |
| `backtrace` | Stack trace (works if frame pointers are preserved) |

### Step 3 — QEMU monitor

While QEMU is running, press `Ctrl-A then C` (with `-serial stdio`) or use `-monitor stdio`. Useful commands:

| Command | Effect |
|---|---|
| `info registers` | Same as GDB's, from QEMU's view |
| `info mem` | Page-table walk |
| `info pic` | Current PIC state (in-service, masked, IRR) |
| `xp /16xw 0x100000` | Physical memory hexdump |
| `screendump <file>` | Save VGA framebuffer as PPM |

Detailed cookbook in [`debugging-guide.md`](debugging-guide.md).

## Commit message format

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short summary>

<longer body, optional>

<footer with breaking-change notes or refs, optional>
```

Examples:

```
feat(scheduler): add nice-value priority hints
fix(irq): send EOI before invoking handler (ADR-0005)
docs(api): add SYS_MEMINFO to syscall table
refactor(vfs): extract path-split helper
test(heap): add fragmentation regression test
chore(ci): bump cppcheck to 2.13
```

| Type | When |
|---|---|
| `feat` | New feature or new public API |
| `fix` | Bug fix |
| `docs` | Documentation-only |
| `refactor` | Code change without behavior change |
| `test` | Adding or fixing tests |
| `chore` | Tooling, build, CI |
| `perf` | Performance improvement |

Scope is typically the subsystem name (`scheduler`, `vmm`, `vga`, `shell`, ...). Skip the scope for project-wide changes.

## Pull requests

1. Branch off `main`: `feat/your-thing` or `fix/your-thing`
2. Run `make format && make lint && make` before pushing
3. PR description should explain *why*, not just *what*
4. If the PR adds an API, update `docs/api/*.md`
5. If the PR changes behavior, add a `CHANGELOG.md` entry
6. If the PR locks down a non-obvious design decision, write an ADR

See [`CONTRIBUTING.md`](../CONTRIBUTING.md) for the full checklist.

## Useful one-liners

```bash
# Watch the boot log in real time
make run 2>&1 | tee boot.log

# Count lines of code
wc -l $(find src include -name '*.c' -o -name '*.h' -o -name '*.asm')

# List all kprintf calls (to grep for noise)
grep -rn 'kprintf\|kputs\|kputc' src/

# Run the heap stress test ONLY (when refactoring heap)
# - temporarily disable everything after `heap_init()` in kmain.c

# Verify the multiboot header is in the right place
objdump -h build/helix.bin | head -20
```
