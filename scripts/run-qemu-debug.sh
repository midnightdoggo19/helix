#!/usr/bin/env bash
# Boot HelixOS under QEMU with GDB stub. Used by `make debug`.
set -euo pipefail
ISO="${1:-build/helix.iso}"
KERNEL_BIN="${2:-build/helix.bin}"

if [ ! -f "$ISO" ]; then
    echo "ISO not found: $ISO — run 'make iso' first"
    exit 1
fi

cat <<EOF
QEMU paused. In another terminal:
    i686-elf-gdb $KERNEL_BIN -ex 'target remote :1234' -ex 'break kmain' -ex 'continue'
EOF

exec qemu-system-i386 \
    -cdrom "$ISO" \
    -m 128M \
    -serial stdio \
    -no-reboot \
    -s -S
