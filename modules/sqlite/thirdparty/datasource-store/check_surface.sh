#!/bin/sh
# Check that the store calls the FoundationDB surface it says it calls.
#
#   check_surface.sh <build-dir>
#
# `foundationdb.allow` beside this script lists the C ABI the store uses and says why each
# absence is an absence. This checks the list against reality, because a list nothing checks
# goes stale and the stale copy still reads as authoritative — the same argument
# `weft/limits.hpp` makes about its transcribed numbers.
#
# Two checks, because there are two ways to leave the surface.
#
# One: a new symbol. Read from the built library rather than from the source, so a call
# reached through a macro or an inline function is still caught. `fdb_select_api_version` is
# exactly that case: the macro expands to `fdb_select_api_version_impl`, and only the
# linker's view knows it.
#
# Two: a snapshot read. That one is not a symbol at all — it is the trailing argument to
# `fdb_transaction_get` and the third argument after the streaming mode in
# `fdb_transaction_get_range`, and it must stay zero. Reading the fence non-snapshot is what
# registers the read conflict that makes FoundationDB reject a write whose fence moved, so a
# snapshot read in the wrong place disables ownership and nothing fails loudly. That is the
# same blind spot `prove_crash` exists for, arriving through the type system's back door.
#
# Why this matters beyond tidiness: `spec/Backend.lean` proves what the layout needs is
# seven laws and not FoundationDB, which is what lets a local backend carry the same store.
# The proof holds only while the C stays inside the surface the laws describe.
#
# SPDX-License-Identifier: Apache-2.0
set -eu

build=${1:-build}
here=$(dirname "$0")
allow="$here/foundationdb.allow"
lib="$build/libweft_fdb_vfs.a"

[ -f "$allow" ] || { echo "check_surface: no $allow"; exit 1; }
[ -f "$lib" ] || { echo "check_surface: no $lib, build weft_fdb_vfs first"; exit 1; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# What the library actually asks the linker for.
nm -u --format=posix "$lib" | awk '$2 == "U" && $1 ~ /^fdb_/ { print $1 }' \
	| sort -u > "$work/actual"

# What we said it would.
sed 's/#.*//' "$allow" | tr -d ' \t' | grep -v '^$' | sort -u > "$work/allowed"

# A library with no FoundationDB in it at all is not a clean surface. It is a check with
# nothing to check, which is the failure `ci.yml` already guards its crash points against:
# a green step that asserted nothing. This becomes reachable the moment the FoundationDB
# backend is optional at build time, and reporting it as nineteen stale entries would bury
# the cause under the symptom.
if [ ! -s "$work/actual" ]; then
	echo "check_surface: $lib references no FoundationDB symbols at all."
	echo "  Either the backend was not built into it, or this is the wrong library."
	echo "  Nothing was checked, so this is a failure and not a pass."
	exit 1
fi

rc=0

# A symbol used and not listed. This is the one that matters: it is a feature entering the
# design without the argument that should come with it.
if unlisted=$(comm -23 "$work/actual" "$work/allowed") && [ -n "$unlisted" ]; then
	echo "check_surface: FoundationDB symbols used but not in foundationdb.allow:"
	echo "$unlisted" | sed 's/^/  /'
	echo "  Add it to the list with the reason, or do not call it."
	rc=1
fi

# A symbol listed and not used. Harmless to run, but it means the list has stopped
# describing the code, and a list that describes nothing is the thing this file exists to
# prevent.
if stale=$(comm -13 "$work/actual" "$work/allowed") && [ -n "$stale" ]; then
	echo "check_surface: listed in foundationdb.allow but no longer used:"
	echo "$stale" | sed 's/^/  /'
	echo "  Remove it, so the list keeps meaning what it says."
	rc=1
fi

# Every point read must pass snapshot = 0, which is the last argument.
if bad=$(grep -n 'fdb_transaction_get(' "$here"/*.c | grep -v ', *0);' || true); [ -n "$bad" ]; then
	echo "check_surface: a point read that is not serializable:"
	echo "$bad" | sed 's/^/  /'
	echo "  The last argument is snapshot, and the fence needs it zero."
	rc=1
fi

# And every range read. After the streaming mode come iteration, snapshot, reverse.
if bad=$(grep -n 'FDB_STREAMING_MODE_' "$here"/*.c \
		| grep -v 'FDB_STREAMING_MODE_[A-Z_]*, *[0-9][0-9]*, *0, *[A-Za-z0-9_]*)' || true)
	[ -n "$bad" ]; then
	echo "check_surface: a range read that is not serializable:"
	echo "$bad" | sed 's/^/  /'
	echo "  The argument after iteration is snapshot, and it must be zero."
	rc=1
fi

[ "$rc" -eq 0 ] && echo "check_surface: ok, $(wc -l < "$work/actual") symbols, all serializable"
exit "$rc"
