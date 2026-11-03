#!/bin/sh
# Derive this layout's parallel-commit spec from the vendored authority.
#
#   spec/tla/derive.sh > spec/tla/WeftParallelCommit.tla
#   spec/tla/derive.sh --check          # fails if the committed module has drifted
#
# `spec/ParallelCommit.lean` states the mapping from ParallelCommits' nouns to this
# layout's, and says outright that the mapping "is an argument rather than an observation".
# This is the machinery that makes it an observation: the mapping is a renaming, the
# renaming is executable, and the module it produces is checked against the committed one by
# a command rather than by a reader.
#
# Why a renaming and not a transcription. A hand-written spec "in the same spirit" is a
# second spec, and the interesting question — whether the two protocols are the same
# protocol — becomes a reading exercise over two files that drift. A renaming cannot drift:
# every action, every label, every invariant is the authority's, with different words on it.
# If the two disagree about anything except vocabulary, this script produces a diff.
#
# What this does NOT establish, stated here because the gap is easy to lose. It shows the
# *specification* is ParallelCommits under new names. It says nothing about whether
# `fdb_vfs.c` implements the specification — that is what `spec/ParallelCommit.lean` argues
# and what `prove_parallel_commit` tests at eight crash points. A renaming is exact about
# the thing it renames and silent about everything below it.
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
src=$here/../../thirdparty/tla/ParallelCommits.tla
out=$here/WeftParallelCommit.tla

# The authority's sha256, so a change to the vendored file is caught here rather than
# producing a silently different derivation. `thirdparty/tla/README.md` records the same
# hash beside the provenance.
expect=2ecc5f7fa6b2a575d12d8ad79a572ef78b8729b2326191b04dafa0ebb6bf15bd
actual=$(sha256sum "$src" | cut -d' ' -f1)
if [ "$actual" != "$expect" ]; then
	echo "derive.sh: the vendored ParallelCommits.tla is not the one this renaming was written against" >&2
	echo "  expected $expect" >&2
	echo "  actual   $actual" >&2
	echo "Re-read the diff before updating the hash: a changed authority may rename nothing" >&2
	echo "and mean something else." >&2
	exit 1
fi

# The mapping, in one place. Each line is the same substitution `spec/ParallelCommit.lean`
# tabulates in prose.
#
#   KEYS          -> DBS          a participant is a database, not a key
#   PREVENTERS    -> RECOVERERS   "preventer" names what it stops; recovery is what runs
#   intent_writes -> staged       pages staged under a txid above the head
#   tscache       -> fence        the one mechanism that refuses a stale writer
#   record        -> txn          weft/txn/<txnid>
#   ts            -> txid         the field: a commit's identity, not a wall clock
#   resolved      -> reclaimed    the staged pages are gone, whichever way they went
#
# `ts` is renamed only where it stands alone as a field. `\b` treats `_` as a word
# character, so `prevent_ts`, `txn_ts` and `query_ts` are untouched — those are the
# algorithm's own locals and renaming them would be renaming variables, not vocabulary.
#
# The renaming is applied to the declarations and to the algorithm, and not to the
# authority's header comment. That comment is 39 lines of prose about CockroachDB's
# implementation files, and renaming inside it produces sentences like "mark its transaction
# txn as committed" while shifting every `*)` off the column it is aligned to. The header
# below replaces it and says what this module is; the prose it replaces is still readable in
# `thirdparty/tla/ParallelCommits.tla`, which is the point of vendoring the file.
rename() {
	sed -e 's/\bMODULE ParallelCommits\b/MODULE WeftParallelCommit/' \
	    -e 's/\bKEYS\b/DBS/g' \
	    -e 's/\bPREVENTERS\b/RECOVERERS/g' \
	    -e 's/\bpreventer\b/recoverer/g' \
	    -e 's/\bintent_writes\b/staged/g' \
	    -e 's/\btscache\b/fence/g' \
	    -e 's/\brecord\b/txn/g' \
	    -e 's/\bts\b/txid/g' \
	    -e 's/\bresolved\b/reclaimed/g' \
	    -e 's/\]_intent_writes/]_staged/g' \
	    -e 's/\]_tscache/]_fence/g' \
	    -e 's/\]_record/]_txn/g'
}
# The three `]_var` rules are not redundant with the `\b` rules above them, and the reason
# is worth stating because it is invisible until it bites. A TLA+ subscript is written
# `[][Action]_vars`, so the variable arrives preceded by an underscore — and `_` is a word
# character, which means `\bintent_writes\b` does not match inside `]_intent_writes`. The
# first version of this script renamed every occurrence except the subscripts, producing a
# module whose temporal properties were stuttering-invariant over variables that no longer
# existed. TLC catches that as an unknown identifier, but only after the module parses far
# enough to reach it.

# Declarations, our header, then the algorithm. `algo` is the line the PlusCal block opens
# on, found rather than hard-coded so a change above it does not silently truncate the
# module.
render() {
	algo=$(grep -n '^(\*--algorithm' "$src" | cut -d: -f1)
	[ -n "$algo" ] || { echo "derive.sh: no PlusCal block in $src" >&2; exit 1; }

	sed -n '1,6p' "$src" | rename
	cat <<'HEADER'

(*************************************************************************)
(* This module is `thirdparty/tla/ParallelCommits.tla` with this layout's *)
(* vocabulary substituted for CockroachDB's. It is produced by            *)
(* `spec/tla/derive.sh`, and `./ci.sh tla` fails if the committed file is *)
(* not what that script emits, so the two cannot drift apart.             *)
(*                                                                       *)
(* The substitution, which is the mapping `spec/ParallelCommit.lean`      *)
(* tabulates:                                                            *)
(*                                                                       *)
(*   KEYS          -> DBS         a participant is a database, not a key *)
(*   PREVENTERS    -> RECOVERERS  recovery is what the process runs      *)
(*   intent_writes -> staged      pages under a txid above the head      *)
(*   tscache       -> fence       what refuses a stale writer            *)
(*   record        -> txn         weft/txn/<txnid>                       *)
(*   ts            -> txid        a commit's identity, not a clock       *)
(*   resolved      -> reclaimed   the staged pages are gone              *)
(*                                                                       *)
(* Nothing else is changed. Every action, label, invariant and temporal  *)
(* property below is the authority's, so a disagreement between the two  *)
(* protocols would appear as a diff rather than as a reading exercise.   *)
(*                                                                       *)
(* What this does not establish: that `fdb_vfs.c` implements the module. *)
(* A renaming is exact about what it renames and silent about everything *)
(* below it. `prove_parallel_commit` tests the C at eight crash points   *)
(* and `spec/ParallelCommit.lean` argues the layout properties this      *)
(* module cannot state, having no pages.                                 *)
(*                                                                       *)
(* The authority's own header, 39 lines on CockroachDB's implementation  *)
(* files and its safety and liveness claims, is left where it is.        *)
(*************************************************************************)
HEADER
	sed -n "${algo},\$p" "$src" | rename
}

if [ "${1:-}" = "--check" ]; then
	if render | diff -u "$out" - >/dev/null 2>&1; then
		echo "tla: WeftParallelCommit.tla is the renaming of ParallelCommits.tla"
		exit 0
	fi
	echo "tla: WeftParallelCommit.tla has drifted from the renaming" >&2
	render | diff -u "$out" - >&2 || true
	exit 1
fi

render
