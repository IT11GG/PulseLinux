#!/usr/bin/env bash
# Run clang-format in-place on all C/H files in src/.
# Usage: bash scripts/format.sh [--check]
set -euo pipefail
CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
MODE="${1:-}"
find src -name '*.c' -o -name '*.h' | while read -r f; do
  if [[ "$MODE" == "--check" ]]; then
    "$CLANG_FORMAT" --dry-run --Werror "$f"
  else
    "$CLANG_FORMAT" -i "$f"
    echo "formatted: $f"
  fi
done
