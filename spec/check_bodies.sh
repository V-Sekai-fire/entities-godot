#!/bin/sh
# Hold `spec/ViewCoherence.lean` against `fdb_vfs.c`.
#
#   check_bodies.sh
#
# `ViewCoherence.lean` proves that every file-scoped transaction body checks its handle's view,
# over a list of bodies written by hand. A body added to the C and not to the list is outside
# the proof while the proof still passes, which is the failure a hand-copied list always
# has. So the list is read back out of both files and compared.
#
# SPDX-License-Identifier: Apache-2.0
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

grep -oE '^static fdb_error_t [a-z_]+_body\(' "$here/../fdb_vfs.c" \
	| sed -E 's/^static fdb_error_t ([a-z_]+)\(/\1/' | sort -u > "$work/c"
grep -oE '^  \| [a-z_]+_body$' "$here/ViewCoherence.lean" \
	| sed -E 's/^  \| //' | sort -u > "$work/lean"

if ! diff -u "$work/c" "$work/lean"; then
	echo "check_bodies: fdb_vfs.c and spec/ViewCoherence.lean disagree on the transaction bodies"
	exit 1
fi
n=$(wc -l < "$work/c" | tr -d ' ')
if [ "$n" -eq 0 ]; then
	echo "check_bodies: found no transaction bodies at all; nothing was checked"
	exit 1
fi
echo "check_bodies: $n transaction bodies, listed in both"
