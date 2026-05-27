# ADR-0001: Implementation language is C + NASM

**Status**: Accepted
**Date**: 2026-05-25
**Deciders**: Project author

## Context

HelixOS is a freestanding operating-system kernel targeting x86. The language must be able to:

- Run in a hosted-libc-free environment
- Express direct memory access, port I/O, and inline assembly
- Be reviewable by people familiar with systems programming
- Have a mature cross-compilation story for i686-elf

Candidates considered: C, C++, Rust, Zig.

## Decision

**Primary language: C (C11, freestanding via `-ffreestanding -nostdlib`).**
**Assembly: NASM** for the handful of files that need it (boot stub, IDT load, ISR/IRQ trampolines, context switch, syscall entry).

## Considered alternatives

### C++

Workable — Haiku, IncludeOS, and parts of NT are written in it. But for an *educational* kernel, the constant-vigilance overhead is real:

- Must disable exceptions (`-fno-exceptions`) and RTTI (`-fno-rtti`)
- Must provide our own `operator new`/`delete` or forbid them
- Templates can produce surprising bloat
- The implicit constructors/destructors hide work that we want visible

The benefits (RAII, type safety, classes) are nice to have but not project-critical. Rejected.

### Rust

Modernized, memory-safe, growing in OS-dev. Strong points:

- Borrow checker prevents whole classes of bugs
- `#![no_std]` is well-supported
- Excellent tutorials exist (Phil Oppermann's blog)

Concerns:

- Steeper learning curve for reviewers who aren't already Rust-fluent
- Smaller ecosystem of OS-dev examples (most things still translate from C tutorials)
- For a *portfolio piece* meant to demonstrate systems mastery, C is more legible — the unsafe parts are the interesting parts, and C makes them visible

Rust is a strong runner-up. A v2.0 rewrite is a defensible follow-up project.

### Zig

Has good qualities (better C interop than Rust, no hidden control flow), but the language is too young — toolchain stability and community resources lag well behind C and Rust. Rejected pending more maturity.

### Python (or any GC'd language)

Cannot run on bare metal. Could only express an OS *simulator*, which dramatically reduces the project's value. Rejected.

## Consequences

**Positive**

- Largest body of reference material (OSDev wiki, xv6, Linux source, every textbook)
- Mature cross-compilation: `i686-elf-gcc` is a well-trodden build
- No "language plumbing" hides what the kernel is doing
- The same skills transfer directly to Linux/BSD kernel work

**Negative**

- No memory safety: pointer bugs are entirely on us
- No RAII: cleanup is manual (mitigated by keeping subsystems small)
- `-Werror` and clang-tidy are mandatory, not optional

**Mitigations**

- Every public function documents its allocation behavior
- The heap has a magic-byte canary checked on every `kfree`
- Static analysis (`cppcheck`) runs in CI
- Every interrupt path is reviewed for sign-extension / off-by-one
