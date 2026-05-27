#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
#  tests/integration/lib.sh
#  Shared helpers for QEMU lifecycle in smoke tests.
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ISO="$ROOT/build/helix.iso"
SERIAL_LOG="/tmp/helix-integration-$$.log"

cleanup() {
    if [ -n "${QEMU_PID:-}" ]; then
        kill -9 "$QEMU_PID" 2>/dev/null || true
        wait "$QEMU_PID" 2>/dev/null || true
    fi
    rm -f "$SERIAL_LOG"
}
trap cleanup EXIT

boot_helix_with_timeout() {
    local timeout="${1:-8}"
    [ -f "$ISO" ] || { echo "ISO missing: $ISO. Run 'make iso' first." >&2; exit 2; }

    qemu-system-i386 \
        -cdrom "$ISO" \
        -m 128M -display none -no-reboot \
        -serial "file:$SERIAL_LOG" &
    QEMU_PID=$!

    sleep "$timeout"

    kill "$QEMU_PID" 2>/dev/null || true
    wait "$QEMU_PID" 2>/dev/null || true
    unset QEMU_PID
}

assert_serial_contains() {
    local pattern="$1"
    if ! grep -qF "$pattern" "$SERIAL_LOG"; then
        echo "── assertion failed: '$pattern' not in serial log ──" >&2
        echo "── serial log ──" >&2
        cat "$SERIAL_LOG" >&2
        echo "── end ──" >&2
        exit 1
    fi
    echo "  ✓ contains: $pattern"
}

assert_serial_lacks() {
    local pattern="$1"
    if grep -qF "$pattern" "$SERIAL_LOG"; then
        echo "── assertion failed: '$pattern' should NOT be in serial log ──" >&2
        cat "$SERIAL_LOG" >&2
        exit 1
    fi
    echo "  ✓ absent:   $pattern"
}
