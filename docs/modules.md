# Module Layout — what's in each folder, and why

The HelixOS source tree is intentionally shallow. Every directory has a single purpose; new code goes in exactly one of them.

```
helix-os/
├── src/
│   ├── boot/           Multiboot entry + linker script
│   ├── kernel/         kmain, panic, version banner
│   ├── arch/i386/      x86 HAL: GDT, IDT, ISRs, IRQs, PIC, port I/O, CPU intrinsics
│   ├── memory/         PMM → paging → VMM → heap
│   ├── process/        task, scheduler, context switch, sync primitives
│   ├── syscalls/       INT 0x80 gate + dispatcher
│   ├── drivers/        one folder per device (vga, serial, timer, keyboard)
│   ├── fs/             vfs.{c,h}, ramfs.{c,h}
│   ├── shell/          shell.{c,h}, commands.c
│   └── lib/            freestanding subset of libc (memcpy/memset/printf/...)
├── include/helix/      kernel-wide headers (types.h, kernel.h, version.h)
├── docs/               this directory tree
│   ├── architecture/   long-form explanations
│   ├── api/            reference (syscalls, kernel-api, drivers)
│   ├── design/adr/     architecture decision records
│   ├── diagrams/       Mermaid sources (.mmd)
│   ├── screenshots/    PNGs of each phase's boot/runtime
│   └── benchmarks/     placeholder for measurement harness
├── scripts/            shell helpers (toolchain build, QEMU launchers)
├── docker/             dev container Dockerfile
├── grub/               grub.cfg used when building the ISO
├── tests/              host unit + integration tests
└── (top-level)         Makefile, README, LICENSE, CONTRIBUTING, etc.
```

## Why this shape

Three principles drove the layout:

1. **One reason per directory.** A folder either contains *hardware abstraction* (`arch/i386/`), *memory management* (`memory/`), or *one device driver* (`drivers/keyboard/`). When you want to add code, the question "where does it go?" has exactly one good answer.

2. **Layer order matches dependency order.** Walking the directory list top-to-bottom roughly walks the dependency graph from leaves to root. `boot/` depends on nothing. `kernel/` depends on everything. Each folder in between depends only on things above it.

3. **Architecture isolated.** Everything x86-specific lives in `src/arch/i386/`. Want to port to ARM? Add `src/arch/arm/` with the same interface and the rest of the tree compiles unchanged. (Today this is partly aspirational — `inb`/`outb` calls leak into a few non-arch files — but the structure makes the deltas reviewable.)

## What goes where — examples

| If you're adding... | It goes in... |
|---|---|
| Code that touches a CPU register or port | `src/arch/i386/` |
| Code that manages physical or virtual memory | `src/memory/` |
| A new device driver | `src/drivers/<name>/` |
| A new syscall implementation | `src/syscalls/syscall.c` |
| A new filesystem | `src/fs/<name>.{c,h}` |
| A new shell command | `src/shell/commands.c` |
| A new freestanding libc function | `src/lib/<header>.{c,h}` |
| A kernel-wide type or macro | `include/helix/kernel.h` |
| Documentation explaining HOW something works | `docs/architecture/` |
| Documentation listing functions | `docs/api/` |
| A locked-down design decision | `docs/design/adr/000N-*.md` |

## What we don't have, and why

| Folder you might expect | Why it doesn't exist |
|---|---|
| `src/net/` | No network stack yet — roadmap item |
| `src/userland/` | No user mode yet — would hold user programs |
| `src/crypto/` | Nothing needs crypto in v0.1 |
| `src/init/` | We have exactly one init task; it's in `kmain.c` |
| `third_party/` | No third-party code in the kernel; we wrote every line |
| `vendor/` | Same |

When these become relevant they'll show up. Premature directories invite premature abstractions.

## File naming conventions

- `<thing>.h` + `<thing>.c` for the C side
- `<thing>.asm` for NASM files (no extension `.s` — `.s` is GAS syntax which we don't use)
- One `static` linkage unit per `.c`; no headers `#include`d from other `.c` files
- Header guards: `HELIX_<DIR>_<NAME>_H` (e.g. `HELIX_PROCESS_TASK_H`)
- `<helix/...>` includes use angle brackets and resolve via `-Iinclude/`
- `"../foo/bar.h"` for sibling-subdirectory includes (yes, relative; the tree is shallow enough that this is fine)

## When to make a new folder

The bar is high. Make a new subdirectory only when:

- You have **at least two** related files going into it
- Those files share more with each other than with anything outside
- The existing folders don't have a natural home

If only one file fits, put it in the closest sibling and revisit when there are more.
