# Memory Model

HelixOS manages memory in four layers, each built on the one below:

```
┌─────────────────────────────────┐
│  kmalloc / kfree                │  ← consumed by every later subsystem
├─────────────────────────────────┤
│  Kernel heap (free-list)        │
├─────────────────────────────────┤
│  VMM (vmm_alloc_region)         │  ← also owns the page-fault handler
├─────────────────────────────────┤
│  Paging (page directory/tables) │
├─────────────────────────────────┤
│  PMM (4 KiB frame bitmap)       │
└─────────────────────────────────┘
                ↑
         Multiboot memory map
        (handed to us by GRUB)
```

Each layer has exactly one job and roughly one or two screen-fulls of code.

## Layer 1 — Physical Memory Manager (PMM)

[`src/memory/pmm.c`](../../src/memory/pmm.c)

### Data structure

A bitmap, one bit per 4 KiB frame. `0 = free, 1 = used`. Placed immediately after the kernel image, aligned to a page boundary. For 128 MiB of RAM the bitmap is 4 KiB — cheap, contiguous, cache-friendly.

```
frame_index  = phys_addr / 4096
byte         = frame_index / 8
bit          = frame_index % 8
```

### Initialization

`pmm_init(multiboot_info_t *mb)`:

1. Walk the Multiboot memory map to find the highest physical address. That bounds `total_frames`.
2. Default the entire bitmap to `0xFF` (everything used). Conservative — we'd rather corrupt nothing than risk handing out a reserved frame.
3. For every `MULTIBOOT_MMAP_AVAILABLE` region, mark its frames free.
4. Re-reserve:
   - The first 1 MiB (legacy BIOS, IVT, VGA buffers)
   - The kernel image (`0x100000` to `kernel_end`)
   - The bitmap itself

After init, `pmm_used_frames` reports how many frames are taken; `pmm_free_frames` is the complement.

### Allocation

`pmm_alloc_frame()` — first-fit linear scan starting from a hint that drifts forward with allocations and backwards on `pmm_free_frame`. Worst case O(n) on the bitmap; amortized much better once the working set stabilizes.

`pmm_alloc_frames(count)` — scans for a run of `count` consecutive zero bits. Used by paging when allocating page tables in bulk (it never does in v0.1, but the API is ready).

### Limits

- Frames only — no sub-page allocation
- No reference counting
- No NUMA awareness (we're single-CPU)
- The bitmap is statically sized at init; we don't grow it for hotplug

## Layer 2 — Paging

[`src/memory/paging.c`](../../src/memory/paging.c)

### Address translation

Classic 32-bit x86 non-PAE paging. A virtual address is split:

```
31              22 21              12 11             0
 [   PD index   ] [   PT index    ] [   byte offset   ]
       (10)              (10)              (12)
```

Translation walks:
1. `CR3` → page directory
2. `PD[v >> 22]` → physical address of a page table (or PSE 4 MiB page; we don't use that)
3. `PT[(v >> 12) & 0x3FF]` → physical address of the frame
4. Add the low 12 bits as the offset

Each PDE and PTE is 32 bits: the high 20 bits are the physical page-frame number, the low 12 are flags (P, R/W, U/S, PWT, PCD, A, D, PS, G).

### PDE / PTE flags

```c
PAGE_PRESENT   (1 << 0)
PAGE_RW        (1 << 1)
PAGE_USER      (1 << 2)
PAGE_PWT       (1 << 3)   // write-through cache
PAGE_PCD       (1 << 4)   // cache disable
PAGE_ACCESSED  (1 << 5)
PAGE_DIRTY     (1 << 6)   // PTE only
```

### Identity map

The kernel page directory `kernel_pd` is a 4 KiB-aligned array of 1024 PDEs. `paging_init` walks 8 MiB of virtual addresses in 4 KiB steps, calling `paging_map(addr, addr, PAGE_RW)`. The result: VA == PA for every address ≤ 8 MiB.

This region covers everything we need: kernel image (~30 KB), bitmap (~4 KB), early page tables allocated by `paging_map` itself, the heap region at `0x00C00000` (12 MiB? No — wait — the heap is at 0x00C00000 = 12 MiB, which is *outside* the 8 MiB identity map).

So `heap_init` immediately calls `vmm_alloc_region` to map the heap's 1 MiB explicitly. The identity map is only for "stuff that exists before the heap is up."

### Enabling paging

```c
load_cr3((uintptr_t)kernel_pd);   // CR3 = phys addr of PD
enable_paging();                    // CR0 |= 0x80000000  (the PG bit)
```

The moment after `mov %eax, %cr0` retires, the CPU starts translating every fetch through the PD. Because we identity-mapped where the code lives, the next instruction succeeds. If the identity map were wrong, the CPU would page-fault inside the instruction-fetch path and triple-fault — there's no recovering from that.

### TLB management

`paging_map` calls `invlpg(virt)` after writing the PTE to flush the stale TLB entry. `paging_flush_tlb()` reloads CR3 (full flush) — used when changing many entries at once.

## Layer 3 — Virtual Memory Manager (VMM)

[`src/memory/vmm.c`](../../src/memory/vmm.c)

A thin convenience layer. Three responsibilities:

### Region allocation

```c
bool vmm_alloc_region(uintptr_t virt, size_t pages, uint32_t flags);
```

For each requested page: `pmm_alloc_frame()` + `paging_map(virt + i*PAGE_SIZE, phys, flags)`. On any failure, rolls back everything allocated so far.

`vmm_free_region` is the reverse — unmap then `pmm_free_frame`.

### Page-fault handler

Registered with `isr_register(14, page_fault_handler)`. When the CPU faults, it pushes an error code (5 bits encoding "why") and stores the offending virtual address in CR2. Our handler reads both, decodes the error code, and panics with a descriptive banner:

```
[ PAGE FAULT ] virt=0x40000000  eip=0x001022b6  err=0x2
  cause: not-present, write, kernel
```

| err bit | Meaning |
|---|---|
| 0 | 0 = page not present, 1 = protection violation |
| 1 | 0 = read, 1 = write |
| 2 | 0 = kernel, 1 = user |
| 3 | reserved bit set in some PDE/PTE |
| 4 | instruction-fetch fault (NX) |

In v0.1 every page fault is fatal. In v0.2 the handler will distinguish "lazy-mapped userspace stack growth" from "real bug."

## Layer 4 — Kernel heap

[`src/memory/heap.c`](../../src/memory/heap.c)

A classical doubly-linked free-list allocator. Each allocation has a header:

```c
struct block {
    uint32_t       magic;     // 0xC0FFEE42 — corruption canary
    size_t         size;      // payload bytes (not incl. header)
    bool           free;
    struct block  *next;
    struct block  *prev;
};
```

Followed by `size` bytes of payload. The pointer returned to the user is `(uint8_t*)header + sizeof(block)`.

### `kmalloc(size)`

1. Walk the block list from head.
2. First-fit: stop at the first `free` block with `size >= requested`.
3. **Split** if the leftover is `≥ HEAP_MIN_SPLIT` (16 bytes) plus a new header. Otherwise hand out the whole block.
4. Mark used. Return payload pointer.

### `kfree(ptr)`

1. Get the header (subtract `sizeof(block)`).
2. Verify the magic — corruption check. Panic if wrong.
3. Verify `!free` — double-free check. Panic if wrong.
4. Mark free.
5. **Coalesce** with `next` if adjacent and free. Then re-check with `prev` (which may now itself be adjacent to a larger free area).

### Heap layout

```
virt 0x00C00000 ─┬─────────────────────┐
                 │ block header        │
                 │   magic, size, free │
                 ├─────────────────────┤
                 │ payload (size B)    │
                 ├─────────────────────┤
                 │ block header        │
                 │ ...                 │
                 └─────────────────────┘
virt 0x00D00000 ─  (heap end — 1 MiB total)
```

The heap is sized at compile time. Growing it dynamically would mean either (a) increasing `KHEAP_INITIAL_SIZE`, or (b) implementing a sbrk-style extension via more `vmm_alloc_region` calls.

### Stress test on every boot

`heap_init` is followed by 8 allocations of varied sizes, each filled with a distinct byte pattern, verified byte-by-byte, freed in scrambled order, then a single 256 KiB allocation to confirm coalescing actually reclaimed the space. If any byte is corrupt or any allocation fails, the boot panics. This is intentional — we treat the heap as a load-bearing invariant.

## Physical memory map at runtime

```
0x00000000 ┌───────────────────────────────────────┐
           │ IVT / BIOS data area                  │
0x000A0000 ├───────────────────────────────────────┤
           │ legacy VGA frame buffer (0xB8000)     │
0x000C0000 ├───────────────────────────────────────┤
           │ BIOS / option ROMs                    │
0x00100000 ├───────────────────────────────────────┤  1 MiB — kernel base
           │ .multiboot + .text + .rodata          │
           │ .data + .bss (stack inside .bss)      │
kernel_end ├───────────────────────────────────────┤
           │ PMM bitmap (4 KiB rounded up)         │
           │ early page tables                     │
0x00C00000 ├───────────────────────────────────────┤  12 MiB — heap base
           │ kernel heap (1 MiB)                   │
0x00D00000 ├───────────────────────────────────────┤
           │ task stacks (allocated by kmalloc)    │
           │ vfs nodes                             │
           │ ...                                   │
0x08000000 └───────────────────────────────────────┘  128 MiB — QEMU default
```

Anything above 8 MiB is unmapped by default; you reach it only through an explicit `vmm_alloc_region` call.

## Tuning knobs

| What | Where | Default |
|---|---|---|
| Page size | hardware | 4 KiB |
| Heap virtual base | `KHEAP_VIRT_START` in `heap.h` | 0x00C00000 |
| Heap size | `KHEAP_INITIAL_SIZE` in `heap.h` | 1 MiB |
| Identity map size | inner loop in `paging_init` | 8 MiB |
| PMM allocation hint | `next_hint` in `pmm.c` | drifts |
| Task stack size | `TASK_STACK_SIZE` in `task.h` | 8 KiB |

All of these are single-line edits.
