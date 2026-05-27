#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
#  format-check.sh — run clang-format in dry-run mode over all C sources
#  Returns non-zero if any file would be reformatted.
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CLANGFMT="${CLANGFMT:-clang-format}"

if ! command -v "$CLANGFMT" >/dev/null 2>&1; then
    echo "error: $CLANGFMT not found in PATH"
    exit 2
fi

mapfile -t FILES < <(find src include -type f \( -name '*.c' -o -name '*.h' \))

if [ "${#FILES[@]}" -eq 0 ]; then
    echo "no source files found"
    exit 0
fi

echo "checking ${#FILES[@]} files with $CLANGFMT"
$CLANGFMT --dry-run --Werror "${FILES[@]}"
echo "format check: PASS"
