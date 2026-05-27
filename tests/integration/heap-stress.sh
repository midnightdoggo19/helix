#!/usr/bin/env bash
# Verify the heap self-test runs cleanly during boot.
set -euo pipefail
source "$(dirname "$0")/lib.sh"

echo "heap-stress: booting..."
boot_helix_with_timeout 5

assert_serial_contains "heap"
assert_serial_lacks "PANIC"
assert_serial_lacks "corrupted block"
assert_serial_lacks "double-free"

echo "heap-stress: PASS"
