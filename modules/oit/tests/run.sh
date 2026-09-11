#!/usr/bin/env bash
# Runs the OIT test suite and refuses the silent-empty-filter trap:
# doctest reports Status: SUCCESS even when 0 test cases matched, so a
# mis-quoted --test-case pattern can silently green a CI run. This wrapper
# parses the summary line and fails when the matched-count is zero.
set -euo pipefail

BIN="${BIN:-$(git rev-parse --show-toplevel)/bin/godot.macos.editor.arm64}"
FILTER="${FILTER:-*OIT*,*Witness*}"

log=$(mktemp)
trap 'rm -f "$log"' EXIT

"$BIN" --headless --test "--test-case=$FILTER" "$@" | tee "$log"

# doctest summary: `[doctest] test cases: N | N passed | N failed | N skipped`
summary=$(grep -E '^\[doctest\] test cases:' "$log" | tail -1)
matched=$(echo "$summary" | sed -E 's/.*test cases:[[:space:]]+([0-9]+).*/\1/')
failed=$(echo "$summary" | sed -E 's/.*\|[[:space:]]+([0-9]+) failed.*/\1/')

if [[ -z "$matched" ]]; then
    echo "run.sh: could not parse doctest summary" >&2
    exit 2
fi
if [[ "$matched" -eq 0 ]]; then
    echo "run.sh: FILTER '$FILTER' matched 0 test cases (silent-green trap)" >&2
    exit 3
fi
if [[ "$failed" -ne 0 ]]; then
    echo "run.sh: $failed test cases failed" >&2
    exit 1
fi
echo "run.sh: $matched test cases matched, all passed."
