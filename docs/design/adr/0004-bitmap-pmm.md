# ADR-0004: PMM uses a bitmap, not a free-list or buddy

**Status**: Accepted
**Date**: 2026-05-25

## Context

The Physical Memory Manager owns every 4 KiB frame in the machine and hands them out one (or N) at a time. Common implementations:

| Algorithm | Space cost | Alloc cost | Multi-frame alloc | Fragmentation handling |
|---|---|---|---|---|
| Bitmap | 1 bit/frame | O(n) scan | Linear scan for run | Implicit (no metadata to skew) |
| Free list (stack) | sizeof(node) per free frame | O(1) | Hard (no order) | None — coalescing impossible |
| Free list (sorted) | sizeof(node) per free frame | O(log n) | Easy | Coalesce on free |
| Buddy | O(log n) | O(log n) | Excellent | Split/merge powers of 2 |

For 128 MiB of RAM:

- Bitmap: 4 KiB total
- Free-list stack: 32K × 4 bytes = 128 KiB
- Buddy: ~256 KiB for free-area headers

## Decision

**Bitmap allocator.**

## Rationale

The PMM is on the *cold* path. After the heap is up, virtually no one calls `pmm_alloc_frame` directly — paging allocates page tables (rare), and `vmm_alloc_region` allocates batches at region-creation time (rare). The bulk of all allocation goes through `kmalloc`, which uses the heap (not the PMM directly).

So the PMM's allocation latency doesn't matter much. What matters:

- **Space**: 4 KiB total is unbeatable. Free-list nodes would eat almost a megabyte at full RAM.
- **Robustness**: A bit can't be corrupted into a dangling pointer. No node-traversal code to bug-check.
- **Reviewability**: 200 lines of straightforward code. A reviewer who doesn't trust the bitmap can verify it bit-by-bit against the Multiboot memory map.

The slower scan time is fine — we never have more than a few hundred PMM operations in a session.

## Considered alternatives

### Free-list stack

Faster (O(1) alloc), but no way to allocate *contiguous* frames — and paging needs that occasionally. Adding that capability re-introduces an O(n) walk anyway. Rejected.

### Buddy allocator

Excellent for general-purpose allocators (especially in Linux), but:

- More complex to implement and debug correctly (split/merge with metadata)
- Wastes memory on power-of-2 rounding
- Pays off only when you allocate widely-varied sizes — we mostly allocate single frames

If HelixOS later needs to allocate large contiguous regions (e.g. for DMA), buddy becomes attractive. For now, the simple `pmm_alloc_frames(N)` linear scan is enough.

### Two-level (slab on top of bitmap)

Real kernels combine: a slab cache for small fixed-size allocations, a bitmap or buddy underneath. Our heap (`kmalloc`) is already a small-object allocator that sits on top of the PMM's virtual region — so we already have a two-level setup, just with a different second layer.

## Consequences

**Positive**

- Trivial to implement and verify
- 4 KiB total overhead regardless of RAM size
- No node corruption possible — the bitmap is fully self-describing

**Negative**

- O(n) worst case allocation (mitigated by the `next_hint` cursor)
- Multi-frame allocation worst-case scans the entire bitmap
- No locality preference: allocations don't try to stay near each other

**Cursor optimization**: `next_hint` records the index of the most recently allocated frame; allocations start the scan there and wrap around once. On `pmm_free_frame`, the hint is dragged back so the freed region is preferred on the next allocation. This makes typical workloads near-O(1) amortized.

## Future direction

A v0.3 SMP port might introduce per-CPU PMM cache lines (each CPU pre-allocates a small batch). The bitmap stays as the source of truth; the cache just amortizes the lock. A separate v0.4 ADR will cover that.
