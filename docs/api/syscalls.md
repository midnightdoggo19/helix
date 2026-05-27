# System Call Reference

HelixOS exposes 9 syscalls via software interrupt `INT 0x80`. The numbering is HelixOS-native (not Linux-compatible). All syscalls follow the i386 register convention used by historical Linux:

| Register | Use |
|---|---|
| `eax` | syscall number on entry, return value on exit |
| `ebx` | arg 0 |
| `ecx` | arg 1 |
| `edx` | arg 2 |
| `esi` | arg 3 |
| `edi` | arg 4 |

Return value `-1` (== `0xFFFFFFFF` in EAX) indicates failure. There is no `errno` mechanism in v0.1.

The IDT gate at 0x80 has DPL=3 so future user-mode code can call it directly.

## Call table

| #  | Name | Args | Returns | Description |
|----|------|------|---------|-------------|
| 0  | `SYS_EXIT` | — | (does not return) | Terminate current task |
| 1  | `SYS_WRITE` | fd, buf, count | bytes written, or `-1` | Write `count` bytes from `buf` to `fd` (1 = stdout, 2 = stderr) |
| 2  | `SYS_READ` | fd, buf, count | bytes read | Blocking line read from `fd` (only `0` = stdin supported) |
| 3  | `SYS_GETPID` | — | pid | Current task's PID |
| 4  | `SYS_YIELD` | — | 0 | Voluntary CPU surrender |
| 5  | `SYS_SLEEP_MS` | ms | 0 | Block for at least `ms` milliseconds |
| 6  | `SYS_UPTIME` | — | ms | Milliseconds since boot |
| 7  | `SYS_PS` | buf, size | bytes written | List all tasks as `pid name state\n` lines into `buf` |
| 8  | `SYS_MEMINFO` | &used, &total, &free_frames | 0 | Heap + frame stats via three out-pointers |

## Inline wrappers

Defined in [`src/syscalls/syscall.h`](../../src/syscalls/syscall.h):

```c
static inline int32_t syscall0(uint32_t n);
static inline int32_t syscall1(uint32_t n, uint32_t a);
static inline int32_t syscall3(uint32_t n, uint32_t a, uint32_t b, uint32_t c);
```

Example use from the shell:

```c
uint32_t ms = syscall0(SYS_UPTIME);

uint32_t used, total, frames;
syscall3(SYS_MEMINFO,
         (uint32_t)&used, (uint32_t)&total, (uint32_t)&frames);

syscall3(SYS_WRITE, FD_STDOUT, (uint32_t)"hello\n", 6);
```

## Per-syscall details

### `SYS_EXIT (0)`

```
eax = 0
INT 0x80
; does not return
```

Marks the current task `TASK_ZOMBIE` and reschedules. The task's stack is *not* freed in v0.1 (zombies are not reaped — there's no `waitpid` yet). Memory leak is bounded by the number of tasks that have ever existed.

### `SYS_WRITE (1)`

```
eax = 1
ebx = fd        (1 = stdout, 2 = stderr — both go to VGA + serial)
ecx = buf       (kernel-virtual pointer)
edx = count
INT 0x80
; eax = bytes written (== count on success, -1 on bad fd)
```

In v0.1, stdout and stderr both route to `kputc` which fans out to VGA and serial. No buffering, no atomic write — a partially-printed line can be interleaved if another task writes mid-stream (the shell uses `print_mtx` to avoid this).

### `SYS_READ (2)`

```
eax = 2
ebx = fd        (must be 0 = stdin)
ecx = buf
edx = count
INT 0x80
; eax = bytes read (1..count)
```

**Blocking line read.** Returns as soon as `\n` arrives or `count` bytes have been collected. The returned buffer includes the `\n`. Calls `kbd_getchar` in a loop, which yields the CPU between keystrokes.

Not currently used by the shell — `shell_main` calls `kbd_getchar` directly for finer control over echo and backspace. Provided for future use by user-mode programs.

### `SYS_GETPID (3)`

```
eax = 3
INT 0x80
; eax = current task's pid
```

### `SYS_YIELD (4)`

```
eax = 4
INT 0x80
; eax = 0
```

Equivalent to calling `task_yield()` from kernel code. Useful when waiting for a condition: spin → yield → recheck.

### `SYS_SLEEP_MS (5)`

```
eax = 5
ebx = ms
INT 0x80
; eax = 0
```

Granularity is the PIT period (10 ms at 100 Hz). `sleep_ms(1)` actually sleeps 10 ms; `sleep_ms(25)` sleeps 30 ms. Worst-case overshoot: one tick.

### `SYS_UPTIME (6)`

```
eax = 6
INT 0x80
; eax = milliseconds since kmain reached its STI
```

Wraps at ~49 days (2^32 ms). Doesn't account for the few hundred microseconds before STI.

### `SYS_PS (7)`

```
eax = 7
ebx = buf       (kernel-virtual)
ecx = size
INT 0x80
; eax = bytes written
```

Writes a textual table of every task into `buf`:

```
  0 init        READY
  1 shell       RUN
```

Columns are space-padded: pid (right-aligned width 3), name (width 12), state. One row per task, terminated with `\n`. Output is truncated to `size` bytes — caller is responsible for sizing `buf`.

### `SYS_MEMINFO (8)`

```
eax = 8
ebx = &used         (uint32_t out, heap bytes in use)
ecx = &total        (uint32_t out, heap total bytes)
edx = &free_frames  (uint32_t out, PMM free frame count)
INT 0x80
; eax = 0
```

Any of the out-pointers may be `NULL`; those fields are skipped.

## Adding a new syscall

1. Add the number to `syscall.h`: `#define SYS_FOO N`, bump `SYS_MAX`.
2. Write `static int32_t sys_foo(registers_t *r) { ... }` in `syscall.c`.
3. Wire it into the dispatcher table: `[SYS_FOO] = sys_foo`.
4. (Optional) Add an inline wrapper to `syscall.h` for ergonomic callers.
5. Document it here.

The dispatcher's null-handler check (`if (!syscall_table[n])`) means leaving holes in the numbering is safe — they return `-1`.

## What we deliberately *don't* support yet

| Missing | Why |
|---|---|
| `fork` / `exec` | No user-mode process model yet |
| `open` / `close` returning fds | The VFS uses pointers directly; fd table is a v0.2 concern |
| Signals | No asynchronous handler infrastructure |
| `errno` | Returning `-1` is enough for v0.1 |
| `mmap` | Heap is fixed; no file-backed mappings |
| `select` / `poll` | Single blocking input source (keyboard) makes this moot |

These are all queued in [ROADMAP.md](../../ROADMAP.md).
