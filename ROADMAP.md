# Roadmap

This roadmap captures what HelixOS doesn't yet do and the rough order in which we'd add it. Each milestone is a major-or-minor SemVer bump.

## v0.1 — Genesis ✅ (current)

Released 2026-05-26. See [CHANGELOG.md](CHANGELOG.md).

Kernel threads, paging, scheduler, syscalls via `INT 0x80`, RAMFS, interactive shell. No user mode, no disk, no network.

---

## v0.2 — Userland

The biggest architectural addition. Brings ring-3 isolation.

**Why next**: every subsequent feature (ELF loading, multi-user, process accounting) depends on having user mode. Doing it first means we don't have to redesign syscalls later.

### Scope

- **TSS (Task State Segment)** with kernel-stack switching on ring transition
- **`iretd` to ring 3** — the actual mode switch from a syscall return
- **Per-task page tables** so each user process has its own address space
- **`fork`** — copy current task into a new process, COW its page tables
- **`exec`** — replace current address space with one loaded from a file
- **Minimal ELF32 loader** — read a regular file, validate the ELF header, map PT_LOAD segments
- **User-side libc skeleton** — `_start`, `crt0`, syscall stubs, `printf`
- **A second hello-world user program** that the shell can `exec`

### Not in this milestone

- Signals (deferred to v0.4)
- `mmap` (deferred to v0.4)
- Multiuser (no users at all yet — `getuid()` returns 0)

---

## v0.3 — SMP

Multi-CPU.

**Why now**: with user mode in place, the locking story gets a real test. Doing SMP before v0.2 would mean revisiting every kernel lock after we add user-mode contention.

### Scope

- **Boot the application processors** via MP tables or ACPI MADT
- **Per-CPU data**: replace `static` globals with arrays indexed by APIC ID
- **Local APIC** initialization (replaces legacy 8259 for IPC delivery)
- **IPI**-based TLB shootdown
- **Spinlocks become real** (current implementation is cli-based — adds the missing test-and-set on contended paths)
- **Scheduler runs per-CPU**, with a global work-stealing fallback

### Risks

- Every shared kernel data structure becomes a question
- Debugging gets harder; the deterministic single-CPU model is a luxury

---

## v0.4 — Storage + IPC primitives

Closes the obvious feature gaps once user mode and SMP are stable.

### Scope

- **Block device interface** with one concrete driver (ATA PIO is simplest)
- **FAT16 read-only driver** mounted under `/mnt`
- **Block cache** — coalesces sector reads
- **`pipe()`, `dup()`** — gets the shell to support `|`
- **Signals** — `kill`, `signal`, `sigaction`
- **`mmap()` and friends** — at least anonymous and file-backed read-only

---

## v0.5 — x86_64

A ground-up rewrite of `src/arch/i386/` for long mode.

### Scope

- 4-level page tables
- New GDT (and the realization that segments mostly stop existing)
- New IDT (different gate format)
- New ABI (System V x86-64, args in registers)
- 64-bit kernel binary, linker script update
- Rebuild of every assembly file

### Promise to keep

Nothing outside `src/arch/i386/` should change. If that promise breaks, our architecture isolation wasn't real.

---

## v0.6 — Networking

Once everything below is solid.

### Scope

- One NIC driver (RTL8139 is the OSDev-friendly choice; e1000 is more representative of real hardware)
- Ethernet + ARP
- Minimal IPv4 + ICMP (so `ping` works)
- UDP socket interface
- TCP — last because it's a project unto itself

---

## v1.0 — when it's ready

v1.0 means HelixOS:

- Boots on real hardware reliably
- Supports multiple users with login
- Has a persistent filesystem
- Has a network stack
- Has been used by people other than the author for some non-trivial purpose
- Has a v0.x → v1.x migration plan
- Has a maintained ABI documented as such

No timeline.

---

## Things we won't do

To stay honest, the list of explicit non-goals:

- **Graphical UI** — VGA text + serial is enough; GUI would be a separate project
- **POSIX compliance** — we're educational, not a Unix-clone
- **Driver count contest** — we add drivers when they teach something new
- **Becoming a daily driver** — there are real OSes for that

---

## How priorities change

Order can change if:

- A particular milestone becomes blocking for a documented use case
- An external contributor wants to lead a milestone we're not focused on
- We discover something on the way that demands attention

Roadmap changes go through a normal PR with `roadmap:` prefix and the reasoning in the PR description.
