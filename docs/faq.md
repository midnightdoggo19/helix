# Frequently Asked Questions

## About the project

### What is HelixOS?

A 32-bit x86 educational operating system kernel written from scratch in C and NASM. It boots via GRUB, runs entirely in ring 0, multitasks kernel threads under a Round-Robin scheduler, and drops you into an interactive shell with a small filesystem you can browse.

### Why "Helix"?

The name evokes structure — turns of a stable form built from a small alphabet of subsystems. It's also short, googleable, and not already taken by a major OS.

### Is this a real operating system?

It's a real *kernel* in the same sense xv6 is a real kernel: it boots on real hardware, runs real CPU instructions, manages real memory, and handles real interrupts. It is **not** a production OS — no userspace, no disk I/O, no networking, no security model worth the name. It exists to demonstrate the engineering of an OS at the kernel layer.

### Can I use this for my class project?

Yes. MIT license, fork freely. Cite the repo and the techniques (paging, scheduling, IPC) you're explaining. Please do read the code rather than copy-paste — half the value is internalizing why each function is shaped the way it is.

## About the build

### Why a cross-compiler? Why can't I just use my system GCC?

Because your system GCC targets your OS (Linux/macOS) — it makes assumptions about libc, system calls, ABI conventions, and startup files that a freestanding kernel must not inherit. A cross-compiler targets `i686-elf`: no OS, no libc, no surprises.

You *can* fake it with `-m32 -ffreestanding -nostdlib` (and we do for in-sandbox testing), but the dedicated cross-compiler avoids subtle mismatches like "your host's startup objects get linked accidentally."

### Why doesn't the build need root access?

Everything is local: the cross-compiler lives in `$HOME/opt/cross`, the build output goes to `./build/`, QEMU runs as your user. The only step that needs apt/brew is installing the *base* tools (gcc, nasm, qemu, grub-mkrescue). Once those exist, no root.

### Why does the first build take 10 minutes?

The cross-compiler build (`binutils` + `gcc`) is the expensive step, and it only happens once. Subsequent builds are seconds.

### Can I build on Windows?

Use WSL2 with Ubuntu. Pure-Windows builds (MinGW, MSVC) are not supported because they don't ship the GRUB tools.

## About the architecture

### Why i386 and not x86_64?

i386 (32-bit) has:

- Simpler memory model (4 KiB pages, no 4-level page tables, no canonical address requirements)
- Smaller boilerplate in the boot path
- The Multiboot1 protocol works without any 64-bit transition song-and-dance
- Better fit for learning — every concept is present without distracting complications

x86_64 is on the roadmap (v0.5). The transition mostly means rewriting `src/arch/i386/` and the linker script.

### Why monolithic and not microkernel?

See [ADR-0002](design/adr/0002-monolithic-kernel.md). Short answer: at our scope, microkernel IPC machinery would dominate the codebase without enabling anything we want to demonstrate.

### Why Round-Robin scheduling?

See [ADR comments in scheduler-design.md](architecture/scheduler-design.md#why-round-robin-and-not-something-fancier). It's the simplest scheduler that's still preemptive and fair. Useful as a baseline; replaceable later.

### Why GRUB instead of writing my own bootloader?

See [ADR-0003](design/adr/0003-multiboot1.md). Writing your own bootloader teaches you bootloader-writing, not kernel-writing. The portfolio value is in the kernel.

### Why doesn't HelixOS have user mode?

User mode (ring 3) requires a syscall path that switches stacks via the TSS, careful protection of every kernel pointer accessible from a user context, and a proper process model. It's a v0.2 goal. The GDT already has the user-segment slots — we just never `iretd` to ring 3.

### Why a bitmap PMM, not buddy?

See [ADR-0004](design/adr/0004-bitmap-pmm.md). Bitmap is 4 KiB total at full RAM size, can't be corrupted into dangling pointers, and is trivial to verify. The PMM is cold-path; we don't need buddy's logarithmic alloc time.

### Why is there no `errno`?

Returning -1 is enough for v0.1. A real `errno` mechanism requires thread-local storage (which requires user mode or a per-task pointer cell), and we'd be inventing the value categories. We'll add it when there's something to express that -1 can't.

## About running it

### How do I quit QEMU?

If you have a graphical window: close it, or `Ctrl-Alt-Q`.

If you're using `-display none` with `-serial stdio`: `Ctrl-A then X`.

If you're using `-monitor stdio`: type `quit` at the `(qemu)` prompt.

If all else fails: `Ctrl-C` in the terminal that launched QEMU, or `pkill qemu-system-i386`.

### Why is QEMU's window so small?

VGA text mode is 80×25 cells. Each cell is 8×16 pixels. So the natural size is 640×400. QEMU scales it as you resize the window.

### How do I save my session?

You don't, in v0.1. RAMFS lives in RAM. Reboot = blank slate. A persistent FAT16 driver is roadmap.

### Why does typing sometimes feel sluggish?

The shell calls `kbd_getchar`, which yields the CPU between keystrokes. Under heavy preemption (which we don't have) this would be noticeable. In practice the delay is tens of microseconds and invisible.

### The shell shows "command not found" for things that should work. Why?

Check spelling. Commands are case-sensitive. There's no tab-completion. Available commands: `help`, `version`, `clear`, `echo`, `uptime`, `ps`, `mem`, `ls`, `cat`, `touch`, `mkdir`, `rm`, `write`, `reboot`.

## About contributing

### Is HelixOS accepting PRs?

Yes — see [CONTRIBUTING.md](../CONTRIBUTING.md). Bug fixes, doc improvements, new shell commands, and additional unit tests are all welcome. Larger features (new drivers, new schedulers, user mode work) — please open an issue first to align on scope.

### Why so much documentation for so little code?

Because reading the code is necessary but not sufficient to understand *why* it's shaped the way it is. The docs are where the reasoning lives. A reviewer landing on the repo cold can read README → architecture/overview.md → memory-model.md and be ready to read the source with the right mental model.

Also because writing documentation forces you to find the parts of your code that don't make sense, before someone else does.

### Where do I report a bug?

Open an issue on GitHub using the bug-report template. If it's security-sensitive, see [SECURITY.md](../SECURITY.md).

## About the future

### When will user mode land?

v0.2. No firm timeline, but the GDT and syscall gate are already structured for it.

### When will SMP land?

v0.3, after user mode. SMP touches every shared global; doing it before user mode would mean revisiting twice.

### Will there be a 64-bit port?

v0.5. Worth doing once everything else stabilizes.

### Will HelixOS ever run on real hardware?

It already does (Multiboot1 makes this nearly free). We test under QEMU because iteration is faster; the same ISO boots on real PCs from a USB stick made with `dd`.
