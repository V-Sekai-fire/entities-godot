#!/bin/sh
# One command runs the commit stage.
#
#   ./ci.sh                  every stage
#   ./ci.sh build surface    the named stages, in the order given
#   ./ci.sh --list           what the stages are
#
# The workflow in `.github/workflows/ci.yml` calls this script rather than restating it.
# That is the whole reason the file exists: a pipeline that restates the developer's
# commands drifts from them, and the drift is discovered by a green CI over a broken
# checkout, or the reverse. One script, two callers.
#
# What it does not do is install anything. A missing FoundationDB is a failure here, not a
# skip, because a suite that quietly declines to run reads exactly like a suite that
# passed. `deps` is the first stage and it names what is absent.
#
# Stage exit codes are collected rather than aborted on, so one failure does not hide the
# state of everything after it. The summary at the end is the report, and the script's own
# status is the number of stages that failed.
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

BUILD=${BUILD:-build}
KEYS_WITNESS_BUILD=${KEYS_WITNESS_BUILD:-build-keys-witness}
RUN=${RUN:-${TMPDIR:-/tmp}/weft-ci-run}
export WEFT_FDB_CLUSTER_FILE=${WEFT_FDB_CLUSTER_FILE:-/etc/foundationdb/fdb.cluster}

ALL='deps build surface handoff fence-read clock integrity big-commit parallel-commit crash vfs-ext keys-witness backup-fresh spec tla'

if [ "${1:-}" = "--list" ]; then
	echo "$ALL" | tr ' ' '\n'
	exit 0
fi

want=${*:-$ALL}

# The summary is accumulated as text rather than printed as it goes, so the end of a long
# run says what happened without scrolling back through the build output.
summary=''
failed=0
note_result() {
	summary="$summary$1 $2 $3
"
	[ "$2" = FAIL ] && failed=$((failed + 1))
	return 0
}

run_stage() {
	name=$1
	printf '\n========== %s\n' "$name"
	# Back to the source tree before every stage. Two stages `cd` into the run directory
	# without a subshell, and a stage that moves the shell moves every stage after it: the
	# first all-stages run resolved `keys-witness`, `spec/tla/derive.sh` and the Lean package
	# against /tmp, failing three stages that were fine.
	#
	# It hid because the workflow splits these across four jobs, so the one arrangement that
	# reaches it is the arrangement this file exists to provide -- the whole stage list as a
	# single command. Resetting here covers the two and any stage added later, which naming
	# the two would not.
	cd "$here"
	start=$(date +%s)
	rc=0
	"stage_$(echo "$name" | tr - _)" || rc=$?
	elapsed=$(($(date +%s) - start))
	if [ "$rc" = 0 ]; then
		note_result "$name" PASS "${elapsed}s"
	else
		note_result "$name" FAIL "${elapsed}s (exit $rc)"
	fi
}

# The fault fixtures are EXCLUDE_FROM_ALL, so the stages that spawn them ask for them. See
# the comment in CMakeLists.txt for why they are not in the default build: each one either
# needs a second process or kills itself, so neither can be an in-process test case, and
# building them for a developer who only wants the store is work nobody asked for.
#
# Built once and reused, because three stages want the same three binaries.
fixtures_built=0
need_fixtures() {
	[ "$fixtures_built" = 1 ] && return 0
	cmake --build "$BUILD" --target fixtures 		-j"${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}" || return 1
	fixtures_built=1
}

# --- the stages -------------------------------------------------------------------------

# What must already be here. Each absence is named, and the count is printed even when it
# is zero, so "nothing was missing" and "the check did not run" do not look alike.
stage_deps() {
	missing=0
	for tool in cmake fdbcli; do
		command -v "$tool" >/dev/null 2>&1 || { echo "missing: $tool"; missing=$((missing + 1)); }
	done
	[ -f "$WEFT_FDB_CLUSTER_FILE" ] || {
		echo "missing: $WEFT_FDB_CLUSTER_FILE (set WEFT_FDB_CLUSTER_FILE)"
		missing=$((missing + 1))
	}
	if command -v fdbcli >/dev/null 2>&1; then
		if fdbcli --exec 'status minimal' 2>&1 | grep -q 'The database is available'; then
			echo "FoundationDB: available"
		else
			echo "missing: a FoundationDB that answers 'status minimal'"
			missing=$((missing + 1))
		fi
	fi
	echo "absent preconditions: $missing"
	[ "$missing" = 0 ]
}

stage_build() {
	cmake -S . -B "$BUILD" -DCMAKE_BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}
	cmake --build "$BUILD" -j"${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
}

# The store calls the FoundationDB surface it says it calls, and every read is
# serializable. Needs no cluster: it reads the built library's undefined symbols.
stage_surface() {
	./check_surface.sh "$BUILD"
}

# A handoff copies nothing. The reader is a different process with no local file, so this
# fails if the VFS ever writes one.
stage_handoff() {
	need_fixtures || return 1
	mkdir -p "$RUN"
	(
		cd "$RUN"
		"$here/$BUILD/prove_handoff" write zone-atlantis.db
		test ! -f zone-atlantis.db || { echo "a local file appeared"; exit 1; }
		"$here/$BUILD/prove_handoff" read zone-atlantis.db
	)
}

# SQLite's own audit of its B-tree, over a database that lives in FoundationDB.
# A stale handle is refused a read, and the fixture fails on the VFS with the check planted
# out. The second run is the negative control: a check that passes on known-broken input
# certifies the defect, so a control that passes is itself the failure.
stage_clock() {
	need_fixtures || return 1
	mkdir -p "$RUN"
	(
		cd "$RUN"
		"$here/$BUILD/prove_clock" zone-clock.db || exit 1
		if WEFT_PLANT_UNIX_CLOCK=1 "$here/$BUILD/prove_clock" zone-clock-plant.db > plant.txt 2>&1; then
			cat plant.txt
			echo "the clock fixture passed with the clock planted back on \"unix\"->xCurrentTime; it checks nothing"
			exit 1
		fi
		echo "control: with WEFT_PLANT_UNIX_CLOCK=1 the clock fixture fails, as it must"
	)
}

stage_fence_read() {
	need_fixtures || return 1
	mkdir -p "$RUN"
	(
		cd "$RUN"
		"$here/$BUILD/prove_fence_read" writer zone-fence-writer.db
		"$here/$BUILD/prove_fence_read" reader zone-fence-reader.db
		"$here/$BUILD/prove_fence_read" reader-exclusive zone-fence-reader-x.db
		for mode in writer reader; do
			if WEFT_PLANT_STALE_VIEW=1 "$here/$BUILD/prove_fence_read" "$mode" "zone-fence-plant-$mode.db" \
				> plant.txt 2>&1; then
				cat plant.txt
				echo "the $mode fixture passed on a VFS with the view check planted out; it checks nothing"
				exit 1
			fi
			echo "control: with WEFT_PLANT_STALE_VIEW=1 the $mode fixture fails, as it must"
		done
	)
}

stage_integrity() {
	mkdir -p "$RUN"
	(cd "$RUN" && "$here/$BUILD/integrity" zone-atlantis.db)
}

# The only program that reaches the staging path, where a commit is too large for one
# transaction.
stage_big_commit() {
	mkdir -p "$RUN"
	(cd "$RUN" && "$here/$BUILD/prove_big_commit" big.db 2000 8192)
}

# A grant across two databases lands whole or not at all. The crash points that matter are
# between staging the intents and writing the record, where the group must abort, and
# between the record and moving the heads, where it is already implicitly committed and
# recovery must finish it. Conservation is the oracle, since integrity_check cannot see two
# databases disagreeing.
#
# The points start past setup: a writer seeds the world before the first grant, so a crash
# below about commit 8 leaves nothing to conserve. A run where no point reached a seeded
# world asserted nothing and fails.
stage_parallel_commit() {
	need_fixtures || return 1
	mkdir -p "$RUN"
	cd "$RUN"
	conserved=0
	for point in 8 10 12 14 17 20 25 30; do
		echo "=== crash before write transaction $point ==="
		rc=0
		out=$("$here/$BUILD/prove_parallel_commit" "world-$point.db" "avatar-$point.db" 8 at "$point" 2>&1) || rc=$?
		echo "$out"
		if [ "$rc" = 1 ]; then echo "a grant was partial at $point"; return 1; fi
		case "$out" in *conserved:*) conserved=$((conserved + 1)) ;; esac
	done
	echo "crash points that reached a seeded world: $conserved of 8"
	if [ "$conserved" = 0 ]; then
		echo "every crash point landed in setup, so this stage asserted nothing"
		return 1
	fi
}

# A crash must leave the whole commit or none of it. Walk crash points rather than one,
# since the fault lives at a particular point and a single delay rarely lands inside a
# commit. The full space is the Lean search in `witness/`, too long for a pull request.
#
# 2000 rows, because at 400 the same points found a planted torn commit in 2 to 7 of 8
# runs, and at 2000 in 8 of 8 (RFD 2323). The second walk is the negative control: with
# the torn commit planted, some point must tear.
crash_walk() {
	torn=0
	seeded=0
	for point in 6 8 10 13 16 21; do
		echo "=== $1: crash before commit $point ==="
		# Exit 2 means the crash point was past the end of the sequence, which is not a
		# fault, so the status is captured rather than left to abort the stage.
		# Not `rc`: run_stage reads that name after the stage returns.
		crc=0
		# A name per run: a database torn by an earlier planted walk fails the writer, and
		# that failure would count as a tear the plant did not cause.
		out=$("$here/$BUILD/prove_crash" "crash-$1-$$-$point.db" 2000 at "$point" 2>&1) || crc=$?
		echo "$out"
		[ "$crc" = 1 ] && torn=$((torn + 1))
		case "$out" in *"rows: 0"*) ;; *"rows: "*) seeded=$((seeded + 1)) ;; esac
	done
}

stage_crash() {
	need_fixtures || return 1
	mkdir -p "$RUN"
	cd "$RUN"
	crash_walk store
	if [ "$torn" != 0 ]; then echo "torn at $torn of 6 crash points"; return 1; fi
	echo "crash points that reached a seeded database: $seeded of 6"
	if [ "$seeded" = 0 ]; then
		echo "every crash point landed in setup, so this stage asserted nothing"
		return 1
	fi
	export WEFT_PLANT_TORN_COMMIT=1
	crash_walk plant
	unset WEFT_PLANT_TORN_COMMIT
	if [ "$torn" = 0 ]; then
		echo "the walk found no tear with a torn commit planted, so it checks nothing"
		return 1
	fi
	echo "control: with WEFT_PLANT_TORN_COMMIT=1, $torn of 6 crash points tear, as they must"
}

# The VFS loaded as an extension into a SQLite shell that does not link it, the way exqlite
# loads it. Rows must outlive the process and be FoundationDB keys, not a local file.
stage_vfs_ext() {
	sq=${SQLITE3:-sqlite3}
	command -v "$sq" >/dev/null 2>&1 || { echo "missing: $sq (the SQLite shell)"; return 1; }
	[ -f "$BUILD/CMakeCache.txt" ] || cmake -S . -B "$BUILD" -DCMAKE_BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo} || return 1
	cmake --build "$BUILD" --target weftfdb || return 1
	ext=$(cd "$BUILD" && pwd)/weftfdb
	db=ci_ext_$$
	open="PRAGMA locking_mode=EXCLUSIVE; PRAGMA journal_mode=MEMORY;"
	count() { "$sq" :memory: ".load $ext" ".open file:$db?vfs=weft_fdb" "$open" "SELECT count(*) FROM t;" | tail -1; }

	if "$sq" "file:$db?vfs=weft_fdb" 'SELECT 1' >/dev/null 2>&1; then
		echo "control failed: weft_fdb opened without the extension loaded"; return 1
	fi
	echo "ok   control: without the extension there is no weft_fdb VFS"

	"$sq" :memory: ".load $ext" ".open file:$db?vfs=weft_fdb" "$open" \
		"CREATE TABLE t(k INTEGER PRIMARY KEY, v TEXT);" \
		"WITH RECURSIVE c(x) AS (SELECT 1 UNION ALL SELECT x + 1 FROM c WHERE x < 300) INSERT INTO t(v) SELECT 'row ' || x FROM c;" >/dev/null || return 1
	n=$(count)
	[ "$n" = 300 ] || { echo "a second process read $n rows, not 300"; return 1; }
	echo "ok   300 rows written through the extension read back from a second process"

	keys=$(fdbcli -C "$WEFT_FDB_CLUSTER_FILE" --exec "getrangekeys weft/db/$db/ weft/db/${db}0 100" 2>/dev/null | grep -c "weft/db/$db/")
	[ "$keys" -gt 0 ] || { echo "no FoundationDB keys under weft/db/$db/"; return 1; }
	[ ! -e "$db" ] || { echo "a local file named $db exists, so the VFS wrote to disk"; return 1; }
	echo "ok   the rows are $keys FoundationDB keys under weft/db/$db/, and no local file"

	out=$("$sq" :memory: ".load $ext" ".open file:$db?vfs=weft_fdb" "$open" \
		".connection 1" ".open file:$db?vfs=weft_fdb" "$open" \
		".connection 0" "INSERT INTO t(v) VALUES('stale writer');" 2>&1)
	case "$out" in
	*readonly*) ;;
	*) echo "control failed: a superseded connection's write was not refused: $out"; return 1 ;;
	esac
	n=$(count)
	[ "$n" = 300 ] || { echo "after the refused write the table holds $n rows, not 300"; return 1; }
	echo "ok   control: a second open raises the fence, and the first connection's write is refused"

	# The shell stops at its first failing SQL argument, so the refused reader write runs last.
	fence() { fdbcli -C "$WEFT_FDB_CLUSTER_FILE" --exec "get weft/db/$db/FENCE" 2>/dev/null | tail -1; }
	before=$(fence)
	"$sq" :memory: ".load $ext" ".open file:$db?vfs=weft_fdb&mode=ro" "SELECT count(*) FROM t;" >/dev/null || return 1
	after=$(fence)
	[ -n "$before" ] && [ "$before" = "$after" ] || { echo "a read-only open moved the fence: $before -> $after"; return 1; }
	out=$("$sq" :memory: ".load $ext" ".open file:$db?vfs=weft_fdb" "$open" \
		".connection 1" ".open file:$db?vfs=weft_fdb&mode=ro" "SELECT count(*) FROM t;" \
		".connection 0" "INSERT INTO t(v) VALUES('writer after a reader');" \
		".connection 1" "INSERT INTO t(v) VALUES('reader');" 2>&1)
	case "$out" in
	*readonly*) ;;
	*) echo "a read-only connection's write was not refused: $out"; return 1 ;;
	esac
	n=$(count)
	[ "$n" = 301 ] || { echo "with a reader attached the writer's insert left $n rows, not 301:"; echo "$out"; return 1; }
	echo "ok   a read-only open leaves the fence alone, its own write is refused, and the writer keeps writing"

	fdbcli -C "$WEFT_FDB_CLUSTER_FILE" --exec "writemode on; clearrange weft/db/$db/ weft/db/${db}0" >/dev/null 2>&1 || true
}

# `fdb_keys.h` is pure, which is why it was split out: this needs neither FoundationDB nor
# SQLite. Ten properties + nine paired rule-2 planted-false controls, over witness-cpp and
# doctest; header-only, no network fetch of a fuzzer framework, exception-free build.
stage_keys_witness() {
	cmake -S keys-witness -B "$KEYS_WITNESS_BUILD" -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build "$KEYS_WITNESS_BUILD" -j"${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
	ctest --test-dir "$KEYS_WITNESS_BUILD" --output-on-failure
}

# The probe weftspun-fdb's backup checks read, run against its own controls before a machine
# arms it, so a regression fails here rather than leaving a production check critical.
stage_backup_fresh() {
	sh fly/backup-fresh.sh --self-test
}

# The read-ahead window's safety argument. The only stage that checks a proof rather than a
# run, and the only one that needs Lean.
stage_spec() {
	command -v lake >/dev/null 2>&1 || { echo "missing: lake (elan/Lean toolchain)"; return 1; }
	spec/check_bodies.sh
	(cd spec && lake build)
}

# The parallel-commit protocol, checked against the spec it claims to be. Three assertions,
# and the third is the one that means something.
#
# One: the committed `WeftParallelCommit.tla` is what `derive.sh` emits from the vendored
# authority. A renaming that anyone can edit afterwards is not a renaming.
#
# Two: TLC checks the derived module — the type invariant, `AckImpliesCommit`, and the four
# temporal properties the authority states about its own record, intents and timestamp cache.
#
# Three: TLC checks the *authority* under the same model, and the two runs must report the
# same state counts. Identical counts is what "the same protocol" reduces to here: the
# renaming cannot have added or removed a reachable state. A run of the derived module alone
# would pass just as happily if the renaming had quietly dropped an action.
stage_tla() {
	command -v java >/dev/null 2>&1 || { echo "missing: java"; return 1; }
	jar=${TLA2TOOLS:-$HOME/.local/lib/tla2tools.jar}
	[ -f "$jar" ] || { echo "missing: $jar (set TLA2TOOLS)"; return 1; }

	./spec/tla/derive.sh --check || return 1

	work=$RUN/tla
	rm -rf "$work"; mkdir -p "$work"
	cp spec/tla/WeftParallelCommit.tla spec/tla/MCWeftParallelCommit.cfg "$work/"
	cp thirdparty/tla/ParallelCommits.tla "$work/"
	# The authority's model is this one with the constants named its way, derived here so
	# the two runs cannot be given different models by accident.
	sed -e 's/DBS/KEYS/' -e 's/RECOVERERS/PREVENTERS/' \
		spec/tla/MCWeftParallelCommit.cfg > "$work/MCParallelCommits.cfg"

	tlc() {
		(cd "$work" && java -XX:+UseParallelGC -cp "$jar" tlc2.TLC \
			-config "MC$1.cfg" -workers "${JOBS:-4}" "$1" 2>&1)
	}

	authority=$(tlc ParallelCommits)
	derived=$(tlc WeftParallelCommit)

	for out in "$authority" "$derived"; do
		case $out in
			*"Model checking completed. No error has been found."*) ;;
			*) echo "$out" | tail -30; echo "TLC did not complete cleanly"; return 1 ;;
		esac
	done

	# "N states generated, M distinct states found" is the line both runs must agree on.
	a=$(printf '%s' "$authority" | grep -oE '[0-9]+ states generated, [0-9]+ distinct states found' | tail -1)
	d=$(printf '%s' "$derived"   | grep -oE '[0-9]+ states generated, [0-9]+ distinct states found' | tail -1)
	echo "authority: $a"
	echo "derived:   $d"
	[ -n "$a" ] || { echo "no state count from the authority's run"; return 1; }
	if [ "$a" != "$d" ]; then
		echo "the renaming changed the reachable state space"
		return 1
	fi
	echo "the two state spaces are identical, so the renaming added and removed nothing"
}

# --- run --------------------------------------------------------------------------------

for stage in $want; do
	case " $ALL " in
		*" $stage "*) run_stage "$stage" ;;
		*) echo "no such stage: $stage" >&2; exit 2 ;;
	esac
done

printf '\n========== summary\n'
printf '%s' "$summary" | while read -r name result timing; do
	printf '%-18s %-5s %s\n' "$name" "$result" "$timing"
done
printf 'stages failed: %d\n' "$failed"
exit "$failed"
