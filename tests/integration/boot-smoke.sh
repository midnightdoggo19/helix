#!/usr/bin/env bash
# Verify that HelixOS boots all the way to the shell prompt within
# a reasonable time and prints each expected init stage.
set -euo pipefail
source "$(dirname "$0")/lib.sh"

echo "boot-smoke: booting for 6 seconds..."
boot_helix_with_timeout 6

assert_serial_contains "GDT       initialized"
assert_serial_contains "IDT       initialized"
assert_serial_contains "PIC       initialized"
assert_serial_contains "paging ON"
assert_serial_contains "round-robin scheduler online"
assert_serial_contains "INT 0x80 gate installed"
assert_serial_contains "mounted at /"
assert_serial_contains "spawning shell"
assert_serial_contains "helix:/\$"

echo "boot-smoke: PASS"
