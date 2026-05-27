# Performance

HelixOS is an *educational* kernel — performance is a secondary goal behind correctness, simplicity, and reviewability. But it's worth being honest about where the costs are.

## Compile-time configuration

| Knob | Where | Default | Effect |
|---|---|---|---|
| Optimization level | `Makefile` `CFLAGS` | `-O2` | Reasonable code gen without unpredictable transforms |
| PIT frequency | `pit.h` `PIT_FREQ_HZ` | 100 | Higher = finer sleep, more IRQ overhead |
| Time slice | (implicit: 1 PIT tick) | 10 ms | RR quantum |
| Heap size | `heap.h` `KHEAP_INITIAL_SIZE` | 1 MiB | Fixed; no growth |
| Task stack | `task.h` `TASK_STACK_SIZE` | 8 KiB | Each task |
| Identity-map size | `paging.c` | 8 MiB | Beyond this, must `vmm_alloc_region` |
| Keyboard buffer | `keyboard.c` `KBD_BUF_SIZE` | 128 | Bytes |

## Boot time

| Stage | Approx wall-clock (QEMU on a 2023 laptop) |
|---|---|
| GRUB → `_start` | ~50 ms |
| `_start` → `kmain` | <1 ms |
| All init in `kmain` | ~80 ms (most of it is the heap stress test) |
| Spawn shell, print MOTD | ~10 ms |
| Total to prompt | ~150 ms |

The PIT ticks at 100 Hz, so boot completes well within the second PIT tick — which is why most of the boot log shows `tick=1`.

## Runtime costs (approximate)

These are instruction-count estimates from reading the disassembly, not measured.

| Operation | Cycles |
|---|---|
| `inb` / `outb` | ~30 (slow due to bus access) |
| VGA `putc` (one cell) | ~40 |
| Serial `putc` | ~50-200 (busy-wait for TX-empty) |
| `kprintf` per char | ~80 (VGA + serial fan-out) |
| `kmalloc` (small, no split) | ~100 (header check + list walk) |
| `kfree` (no coalesce) | ~50 |
| `pmm_alloc_frame` (hint hit) | ~30 |
| `pmm_alloc_frame` (full scan) | ~10,000 worst case |
| `paging_map` | ~100 (PTE write + invlpg) |
| Context switch | ~80 (4 pushes/pops + popfd) |
| PIT IRQ entry to handler | ~150 |
| PIT IRQ total (no switch) | ~200 |
| PIT IRQ total (with switch) | ~300 |
| Syscall round trip | ~250 |

For perspective, at 100 Hz preemption with `with switch` cost ≈ 300 cycles, the scheduler eats roughly 0.001% of CPU. The cost of multitasking is essentially the cost of running the dispatcher.

## Memory footprint

| Section | Size |
|---|---|
| `.text` | 29 KiB |
| `.rodata` | small (~1 KiB; folded into .text by linker rounding) |
| `.data` | 4 B |
| `.bss` | 24 KiB (mostly the boot stack + page directory + PMM bitmap + GDT/IDT tables) |

Total memory at idle (kernel + bitmap + heap region mapped): ~1.1 MiB.

Each spawned task adds: PCB (40 B) + stack (8 KiB) ≈ 8 KiB. RAMFS adds a few hundred bytes per file/directory.

## Where the hot spots are

If we ever needed to optimize:

1. **`kprintf`** is the most-called function and not vectorized. Switching to a buffered ring → background flush could help, but adds complexity.
2. **Serial `putc`** busy-waits. An IRQ-driven TX path would free the CPU; we don't because the PIT and keyboard are higher priority.
3. **PMM linear scan** is O(n). Tiered free-lists by size class would help if we did many multi-frame allocs.
4. **Round-Robin scheduler** walks the entire task list on every preemption. With > 100 tasks this would dominate; with our 3 it's free.

## Benchmarks we should add

Once unit testing matures, the benchmarks worth tracking:

- IPC ping-pong between two tasks (when we get user mode)
- Cold vs hot `kmalloc` distributions
- Context-switch latency under stress (1000 yields/sec)
- Time-to-prompt under different GRUB timeouts

For now, this section is aspirational. See [`docs/benchmarks/README.md`](benchmarks/README.md) for the harness when it lands.
