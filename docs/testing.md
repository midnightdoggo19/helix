# Testing

HelixOS has three test layers, each catching a different class of bug.

## Layer 1 — Build-time checks

The compiler is the cheapest gate. We crank every relevant warning to an error:

```make
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -Wshadow -Wmissing-prototypes -Wstrict-prototypes
CFLAGS += -Wpointer-arith -Wbad-function-cast
CFLAGS += -Wcast-align -Wcast-qual
CFLAGS += -Wno-unused-parameter   # tolerable in handler signatures
```

Every PR that fails `make` is rejected by CI before review. This catches: shadowed variables, missing prototypes, implicit truncation, pointer-to-int conversions, dead code, unreachable statements, and a long list of friends.

## Layer 2 — Host-side unit tests

Some kernel modules are pure data structures and can be linked into a regular host binary for fast unit testing. These live under `tests/unit/`.

```bash
make test
```

Currently covered:

| Module | Test file | What it checks |
|---|---|---|
| `lib/string` | `tests/unit/test_string.c` | memcpy/memset/strlen/strcmp edge cases |
| `lib/printf` | `tests/unit/test_printf.c` | format specifier parsing, width, padding |
| `memory/heap` (logic only) | `tests/unit/test_heap.c` | first-fit, split, coalesce on mock buffer |
| `fs/vfs` | `tests/unit/test_vfs.c` | path resolution, recursive create, lookup misses |

Tests build with the **host** compiler (not the cross-compiler) and link with a tiny harness that provides stand-ins for `kmalloc/kfree` (just `malloc/free`) and `kprintf` (just `printf`).

This works for any module whose only external dependencies are libc-equivalent. The hardware-touching modules — paging, IRQ, drivers — can't be unit-tested this way; they go to layer 3.

## Layer 3 — Integration / smoke tests

Boot the actual kernel under QEMU, capture serial output, assert on what we expect. Lives under `tests/integration/`.

```bash
make integration-test
```

The current battery:

| Test | What it asserts |
|---|---|
| `boot-smoke.sh` | Boots to "READY" within 5 seconds |
| `panic-on-div0.sh` | Triggers a divide-by-zero, verifies the panic banner shows up |
| `pagefault-recovery.sh` | Touches an unmapped address, expects the page-fault panic with the correct CR2 |
| `heap-stress.sh` | The built-in heap stress test in `kmain` succeeds |
| `multitask.sh` | Three worker threads complete their iteration counts within the expected wall-clock |
| `shell-commands.sh` | Drives the shell with `ls`, `ps`, `mem`, `uptime`, verifies expected substrings |

Each test is a short shell script that:

1. Boots `qemu-system-i386` with `-serial file:/tmp/log`
2. Optionally pipes commands through `-monitor unix:`
3. After a timeout, kills QEMU
4. `grep`s the log for the expected line
5. Exits 0 on match, 1 otherwise

This is the kind of test that catches regressions like ADR-0005 (the keyboard EOI bug): a smoke test that types `ls` and waits for output would have failed loudly when the regression was introduced.

## Layer 4 — Manual exploratory testing

Some classes of bug don't lend themselves to automation: visual VGA glitches, scheduler unfairness over long runs, timing-sensitive race conditions. The plan:

1. Run `make run` after any substantive change
2. Spend 60 seconds typing into the shell — `ls`, `ps`, `mem`, repeatedly
3. Verify the output matches expectations
4. Verify no panic, no hang, no garbled output

Boring. Necessary.

## Continuous Integration

`.github/workflows/ci.yml` runs every layer on every push:

1. Build (cross-compile)
2. `make lint` (cppcheck)
3. `make format-check` (clang-format dry-run)
4. `make test` (host unit tests)
5. `make integration-test` (boot smoke tests with timeout)

Total runtime: about 3 minutes on a GitHub Actions runner.

## What's NOT tested in v0.1

| Gap | Reason | Plan |
|---|---|---|
| Long-running stability | No CI runner can spend hours | Manual runs |
| SMP correctness | No SMP yet | When we add it |
| Hardware variation | All tests run under QEMU | Manual hardware tests pre-release |
| Power management | No power management | Out of scope |

## Writing a new test

### A new unit test

1. Create `tests/unit/test_<module>.c`
2. Include the module's header normally
3. Include `<tests/harness.h>` for `ASSERT`, `ASSERT_EQ`, `ASSERT_STREQ`
4. Define a `int main(void)` that calls each test function in sequence
5. Add to `Makefile`'s `UNIT_TESTS` variable

Template:

```c
#include "tests/harness.h"
#include "src/lib/string.h"

static void test_strlen_empty(void) {
    ASSERT_EQ(strlen(""), 0);
}

static void test_strlen_basic(void) {
    ASSERT_EQ(strlen("helix"), 5);
}

int main(void) {
    test_strlen_empty();
    test_strlen_basic();
    printf("OK\n");
    return 0;
}
```

### A new integration test

1. Create `tests/integration/<scenario>.sh`
2. Make it executable
3. Use the helpers in `tests/integration/lib.sh` for QEMU lifecycle
4. Assert on serial output via `grep -q`

Template:

```bash
#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib.sh"

boot_helix_with_timeout 5
assert_serial_contains "[ READY ]"
assert_serial_contains "helix:/$"
```

The framework handles cleanup on failure.
