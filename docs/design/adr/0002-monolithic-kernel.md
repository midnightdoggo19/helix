# ADR-0002: Monolithic kernel architecture

**Status**: Accepted
**Date**: 2026-05-25

## Context

There are three common kernel architectures:

| Style | Examples | Trade-off |
|---|---|---|
| Monolithic | Linux, *BSD, classic UNIX | All services in ring 0. Fast IPC (none needed). Single failure domain. |
| Microkernel | L4, Minix, QNX | Minimal ring-0 core; drivers + filesystem in user processes. IPC-heavy. Strong isolation. |
| Hybrid | XNU (macOS), NT | Microkernel framing with monolithic-ish optimizations layered on |

HelixOS needs to choose one to organize its source tree and define its protection model.

## Decision

**Monolithic.** Every subsystem — drivers, scheduler, filesystem — runs in ring 0 and shares the kernel address space.

## Rationale

A microkernel design would mean:

- A real user-mode story (we don't have one in v0.1; ring 3 is a roadmap item)
- IPC machinery (message queues / synchronous send-receive / shared pages)
- Each driver as a separate process, scheduled like any other task
- Heavier syscall surface

That's an enormous amount of additional engineering for a project at HelixOS's scope. It would dominate everything else.

For a kernel that fits in ~5,000 lines, monolithic is correct:

- Subsystem interfaces are direct function calls (cheap, debuggable)
- Memory mapping is simple — everything is in one address space
- Adding a driver = adding a folder, not designing an IPC contract
- The "all in ring 0" property is a known trade-off and we accept it explicitly

## Internal structure within monolithic

We still apply *layering* internally (see [architecture/overview.md](../../architecture/overview.md)):

- HAL (`arch/i386/`) → drivers (`drivers/`) → memory (`memory/`) → process (`process/`) → syscall (`syscalls/`) → fs (`fs/`) → shell (`shell/`)

Each layer can only call into layers below it. This isn't a microkernel boundary — there's no privilege change — but it gives us the maintainability benefit (testable layers, port-friendly) without the IPC cost.

## Consequences

**Positive**

- Inter-subsystem calls are free
- Easy to add new code: no IPC protocol to design
- Faster boot, smaller binary
- The whole kernel fits in a reviewer's head

**Negative**

- A bug in any driver can corrupt any other subsystem's memory
- No protection between kernel components
- Harder to write tests for individual layers (they all share state)

**Compensating measures**

- Heap headers carry magic bytes; corruption is caught at the next `kfree`
- Every kernel data structure is small (the largest, `task_t`, is 40 bytes)
- Module boundaries are enforced by include discipline, reviewed in CI

## Future direction

If/when v0.3 adds SMP, we'll need to revisit locking on every shared global. The decision to be monolithic doesn't preclude this — it just means there are more places to add locks. A microkernel would have fewer locks but only because the locked structures live in user processes with their own consistency model. The total complexity is comparable; we're choosing where to spend it.
