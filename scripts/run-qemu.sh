#!/usr/bin/env bash
# Boot HelixOS under QEMU. Convenience wrapper used by `make run`.
set -euo pipefail
ISO="${1:-build/helix.iso}"

if [ ! -f "$ISO" ]; then
    echo "ISO not found: $ISO"
    echo "Build it first: make iso"
    exit 1
fi

exec qemu-system-i386 \
    -cdrom "$ISO" \
    -m 128M \
    -serial stdio \
    -no-reboot
