# ─────────────────────────────────────────────────────────────────────
#  HelixOS Makefile
#  Targets:
#    make             Build kernel binary -> build/helix.bin
#    make iso         Build bootable ISO  -> build/helix.iso
#    make run         Build ISO and boot under QEMU
#    make debug       Boot QEMU paused, GDB server on :1234
#    make test        Run host-side unit tests
#    make lint        Run cppcheck static analysis
#    make format      Apply clang-format to all C sources
#    make format-check  Verify formatting without modifying
#    make clean       Remove build/
#    make help        Print this help
# ─────────────────────────────────────────────────────────────────────

# Cross compiler (override via env: e.g. CC=gcc make for sandbox builds)
CC      ?= i686-elf-gcc
LD      ?= i686-elf-ld
AS      := nasm
HOSTCC  ?= gcc

# Tools
QEMU    ?= qemu-system-i386
GRUB_MKRESCUE ?= grub-mkrescue
CLANGFMT      ?= clang-format
CPPCHECK      ?= cppcheck

# Directories
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso
TEST_DIR  := tests

# Compiler / assembler / linker flags
CFLAGS  := -std=c11 -m32 \
           -ffreestanding -fno-builtin -nostdlib \
           -fno-stack-protector -fno-pic -fno-omit-frame-pointer \
           -O2 -g \
           -Wall -Wextra -Werror \
           -Wno-unused-parameter \
           -I$(INC_DIR) -I$(SRC_DIR)

ASFLAGS := -f elf32

LDFLAGS := -m elf_i386 -nostdlib -T $(SRC_DIR)/boot/linker.ld

QEMU_FLAGS := -m 128M -no-reboot

# ─── Source discovery ────────────────────────────────────────────────
C_SOURCES   := $(shell find $(SRC_DIR) -name '*.c')
ASM_SOURCES := $(shell find $(SRC_DIR) -name '*.asm')

C_OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJECTS     := $(C_OBJECTS) $(ASM_OBJECTS)

KERNEL_BIN  := $(BUILD_DIR)/helix.bin
ISO_FILE    := $(BUILD_DIR)/helix.iso

# ─── Default ─────────────────────────────────────────────────────────
.PHONY: all
all: $(KERNEL_BIN)

# ─── Help ────────────────────────────────────────────────────────────
.PHONY: help
help:
	@echo "HelixOS build targets:"
	@echo "  make             Build kernel binary"
	@echo "  make iso         Build bootable ISO"
	@echo "  make run         Boot under QEMU"
	@echo "  make debug       Boot QEMU paused with GDB stub"
	@echo "  make test        Run host-side unit tests"
	@echo "  make lint        Run cppcheck static analysis"
	@echo "  make format      Apply clang-format"
	@echo "  make format-check  Verify formatting"
	@echo "  make clean       Remove build artifacts"

# ─── Compile rules ───────────────────────────────────────────────────
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "  AS   $<"
	@$(AS) $(ASFLAGS) $< -o $@

# ─── Link ────────────────────────────────────────────────────────────
$(KERNEL_BIN): $(OBJECTS) $(SRC_DIR)/boot/linker.ld
	@mkdir -p $(BUILD_DIR)
	@echo "  LD   $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "  ─── Build succeeded ───"
	@size $@

# ─── ISO ─────────────────────────────────────────────────────────────
.PHONY: iso
iso: $(ISO_FILE)

$(ISO_FILE): $(KERNEL_BIN) grub/grub.cfg
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/helix.bin
	@cp grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@echo "  ISO  $@"
	@$(GRUB_MKRESCUE) -o $@ $(ISO_DIR) 2>/dev/null

# ─── Run ─────────────────────────────────────────────────────────────
.PHONY: run
run: $(ISO_FILE)
	@echo "  RUN  $(ISO_FILE)"
	$(QEMU) $(QEMU_FLAGS) -cdrom $(ISO_FILE) -serial stdio

.PHONY: run-headless
run-headless: $(ISO_FILE)
	$(QEMU) $(QEMU_FLAGS) -cdrom $(ISO_FILE) -display none -serial stdio

# ─── Debug ───────────────────────────────────────────────────────────
.PHONY: debug
debug: $(ISO_FILE)
	@echo "QEMU paused, GDB stub on :1234"
	@echo "In another terminal: i686-elf-gdb $(KERNEL_BIN) -ex 'target remote :1234'"
	$(QEMU) $(QEMU_FLAGS) -cdrom $(ISO_FILE) -serial stdio -s -S

# ─── Lint / format ───────────────────────────────────────────────────
.PHONY: lint
lint:
	@echo "  CPPCHECK"
	@$(CPPCHECK) --enable=warning,style,performance,portability \
	             --inline-suppr --suppress=missingIncludeSystem \
	             --error-exitcode=1 \
	             -I$(INC_DIR) -I$(SRC_DIR) $(SRC_DIR)

.PHONY: format
format:
	@echo "  FORMAT"
	@find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | \
	    xargs $(CLANGFMT) -i

.PHONY: format-check
format-check:
	@echo "  FORMAT-CHECK"
	@find $(SRC_DIR) $(INC_DIR) -name '*.c' -o -name '*.h' | \
	    xargs $(CLANGFMT) --dry-run --Werror

# ─── Tests ───────────────────────────────────────────────────────────
.PHONY: test
test:
	@echo "  TEST (host-side)"
	@if [ -d $(TEST_DIR)/unit ]; then \
	  $(MAKE) -C $(TEST_DIR)/unit; \
	else \
	  echo "  (no tests/unit yet — placeholder)"; \
	fi

.PHONY: integration-test
integration-test: $(ISO_FILE)
	@echo "  INTEGRATION-TEST"
	@if [ -d $(TEST_DIR)/integration ]; then \
	  for t in $(TEST_DIR)/integration/*.sh; do \
	    echo "  RUN  $$t"; bash $$t || exit 1; \
	  done; \
	else \
	  echo "  (no tests/integration yet — placeholder)"; \
	fi

# ─── Clean ───────────────────────────────────────────────────────────
.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo "  CLEAN"

# ─── Diagnostic ──────────────────────────────────────────────────────
.PHONY: list-sources
list-sources:
	@echo "C sources ($(words $(C_SOURCES))):"
	@for f in $(C_SOURCES); do echo "  $$f"; done
	@echo "ASM sources ($(words $(ASM_SOURCES))):"
	@for f in $(ASM_SOURCES); do echo "  $$f"; done
