#!/bin/sh
set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DIR="$ROOT/thirdparty/predictive_bvh"
BIN="${BIN:-$(mktemp -d)}"
CXX="${CXX:-c++}"
OLD='[h]ilbert'
lint() { grep -r -n -i "$OLD" "$@"; }
build() { "$CXX" -std=c++17 -O2 -I "$ROOT" -o "$BIN/$2" "$1/morton_check.cpp"; }
must_fail() {
	rc=0
	"$@" > "$BIN/control.out" 2>&1 || rc=$?
	tail -2 "$BIN/control.out"
	if [ "$rc" != 1 ]; then
		echo "RESULT: ERROR a control was not caught"
		exit 2
	fi
}

mkdir -p "$BIN/named" "$BIN/h-swapped"
printf 'uint32_t ghost_%s = 0;\n' "hilbert" > "$BIN/named/planted.h"
if ! lint "$BIN/named" > /dev/null; then
	echo "RESULT: ERROR the lint did not find a planted name"
	exit 2
fi
if lint "$ROOT/core" "$ROOT/tests" "$DIR"; then
	echo "RESULT: FAIL a source line names the old curve"
	exit 1
fi

sed 's/(pbvh_part1by2(x) << 2) | (pbvh_part1by2(y) << 1) | pbvh_part1by2(z)/(pbvh_part1by2(z) << 2) | (pbvh_part1by2(y) << 1) | pbvh_part1by2(x)/' "$DIR/predictive_bvh.h" > "$BIN/h-swapped/predictive_bvh.h"
if cmp -s "$DIR/predictive_bvh.h" "$BIN/h-swapped/predictive_bvh.h"; then
	echo "RESULT: ERROR the old axis order was not planted"
	exit 2
fi
cp "$DIR/morton_check.cpp" "$BIN/h-swapped/"
awk 'BEGIN{FS=OFS="\t"} NR==200{$4=$4+1} {print}' "$DIR/morton_golden.tsv" > "$BIN/altered.tsv"
build "$DIR" real
build "$BIN/h-swapped" swapped
must_fail "$BIN/swapped" "$DIR/morton_golden.tsv"
must_fail "$BIN/real" "$BIN/altered.tsv"
"$BIN/real" "$DIR/morton_golden.tsv"
