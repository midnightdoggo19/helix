#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
#  lint.sh — run cppcheck over the source tree
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CPPCHECK="${CPPCHECK:-cppcheck}"

if ! command -v "$CPPCHECK" >/dev/null 2>&1; then
    echo "error: $CPPCHECK not found in PATH"
    exit 2
fi

exec $CPPCHECK \
    --enable=warning,style,performance,portability \
    --inline-suppr \
    --suppress=missingIncludeSystem \
    --suppress=unusedFunction \
    --error-exitcode=1 \
    --quiet \
    -Iinclude -Isrc \
    src
