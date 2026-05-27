# ─────────────────────────────────────────────────────────────────────
#  Unit tests — built with HOST compiler against modules that don't
#  need hardware (string, printf, vfs-logic, heap-logic).
# ─────────────────────────────────────────────────────────────────────

CC      ?= gcc
CFLAGS  := -std=c11 -m32 -O0 -g -Wall -Wextra -Wno-unused-parameter \
           -fno-builtin -ffreestanding \
           -I../../include -I../..

# Tests link with the host's regular stdio for printf in the harness,
# but use the kernel's freestanding types and string headers.

TESTS := test_string test_printf
BINS  := $(addprefix bin/,$(TESTS))

.PHONY: all clean
all: $(BINS)
	@echo "── Running unit tests ──"
	@for t in $(BINS); do \
	    echo "→ $$t"; \
	    ./$$t || exit 1; \
	done
	@echo "── All unit tests passed ──"

bin/test_string: test_string.c ../../src/lib/string.c ../../src/lib/string.h
	@mkdir -p bin
	@$(CC) $(CFLAGS) -o $@ test_string.c

bin/test_printf: test_printf.c ../harness.h
	@mkdir -p bin
	@$(CC) $(CFLAGS) -o $@ test_printf.c

clean:
	@rm -rf bin
