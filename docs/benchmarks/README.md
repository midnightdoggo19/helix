# Benchmarks

**Status**: placeholder — benchmark harness is roadmap.

This directory will eventually hold:

1. **`harness.c`** — host-side runner that boots HelixOS under QEMU, drives it through a workload, and parses serial output for timing markers.
2. **`workloads/`** — individual benchmark scripts (ping-pong, alloc-heavy, context-switch storm).
3. **`baseline.json`** — recorded numbers from a known-good run.
4. **`results/`** — generated; gitignored.

## Why we don't have this yet

Until v0.1 ships, the project priority has been correctness and clarity. Benchmarks tell you whether a change made things faster — but if the kernel didn't yet *exist*, there was nothing to measure.

Now that v0.1 is in place, the path forward:

| Phase | Output |
|---|---|
| **B1** | Boot-time benchmark: time from GRUB to shell prompt across N runs, report mean ± stddev |
| **B2** | Context-switch latency: spawn 2 tasks that ping-pong via mutex, measure rate |
| **B3** | Heap micro: alloc/free patterns, report cycles per operation |
| **B4** | Syscall round-trip: time `getpid` in a tight loop |
| **B5** | PIT tick overhead: idle for N seconds, count IRQs, verify <1% CPU spent in handler |

## How measurements will work

HelixOS exposes uptime via `pit_uptime_ms()` (millisecond resolution). For finer measurements we'll add `rdtsc` reads — the x86 timestamp counter ticks at the CPU's invariant rate.

The harness will:

1. Boot the kernel with a workload command embedded in `kmain.c` (controlled by a `#define BENCH_*` macro)
2. The workload prints structured lines like `BENCH: ctxswitch_ns=312`
3. The harness greps for `BENCH:` lines, parses, aggregates
4. Compares against `baseline.json`; fails CI if regression > 10%

## Why this matters

For an educational kernel, perf numbers serve two purposes:

1. **Show you actually measure** — saying "the scheduler is fast" is hand-waving; saying "context switch costs 80 ± 4 cycles on QEMU/host" is engineering.
2. **Catch regressions before they ship** — an innocuous refactor that 2× the context-switch cost is the kind of thing a CI benchmark catches but human review doesn't.

When the harness lands, this README will be replaced with the real one.
