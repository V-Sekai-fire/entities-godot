#!/bin/bash
# Prove a FoundationDB cluster tolerates a zone loss, by taking one away.
#
# Stage 1 is the cluster's opinion of itself and proves almost nothing on its own. Stages 3
# to 7 are observations, and they are what the claim rests on: a store that reports fault
# tolerance and then loses data when a zone dies was lying, and only killing the zone finds
# that out. `prove_crash.c` exists for the same reason at process scale.
#
# Every wait names a state and observes it. See `wait.sh` for why there are no sleeps.
#
# Ports 4701-4703, so it never collides with a cluster already running on this machine.
#
# SPDX-License-Identifier: Apache-2.0
set -uo pipefail

SERVER=${SERVER:-fdbserver}
CLI=${CLI:-fdbcli}
ROOT=${ROOT:-$HOME/.local/state/weft-fdb-qa/run}
REDUNDANCY=${REDUNDANCY:-double}
KEYS=${KEYS:-200}
CF="$ROOT/fdb.cluster"
PIDS=()

log() { echo "[qa] $*"; }
fail() {
	echo "[qa] FAIL: $*" >&2
	exit 1
}
cleanup() {
	log "cleaning up"
	for p in "${PIDS[@]:-}"; do kill -9 "$p" 2>/dev/null || true; done
	wait 2>/dev/null || true
}
trap cleanup EXIT

start() { # start <index>
	local i=$1 port=$((4700 + $1))
	"$SERVER" -p "127.0.0.1:$port" -C "$CF" -d "$ROOT/d$i" -L "$ROOT/logs" \
		--locality-machineid "m$i" --locality-zoneid "z$i" >/dev/null 2>&1 &
	local pid=$!
	eval "PID$i=$pid"
	PIDS+=("$pid")
	log "started z$i on $port pid=$pid"
}

# Killing is not stopping. `kill -9` returns at once, but the process still holds its port
# and data directory until the kernel reaps it, so a restart on the same port loses a race
# that a `sleep` used to hide. `wait` blocks until the child is really gone, which is the
# state we need and not a guess at how long it takes.
stop() { # stop <pid...>
	for p in "$@"; do kill -9 "$p" 2>/dev/null || true; done
	for p in "$@"; do wait "$p" 2>/dev/null || true; done
}

faulttol() {
	"$CLI" -C "$CF" --exec 'status' --timeout 30 2>&1 |
		grep -oE 'Fault Tolerance +- +[0-9]+ (zones|machines)' | head -1
}

count_keys() {
	# fdbcli opens its quotes with a backtick: `weft/qa/k0001' is `v1'. Matching a leading
	# apostrophe counted zero, which reads exactly like total data loss.
	"$CLI" -C "$CF" --exec "getrange weft/qa/ weft/qa0 $((KEYS + 10))" --timeout 60 2>/dev/null |
		grep -c "weft/qa/k[0-9]"
}

# 200 sets in one transaction on a freshly configured cluster returns
# commit_unknown_result (1021): distribution is still settling and the batch is one commit.
# Retrying the batch is both the write and the wait — 1021 means the commit may have
# succeeded, so repeating a `set` of the same value is safe.
write_keys() {
	local batch=50 lo hi cmd attempt
	for ((lo = 1; lo <= KEYS; lo += batch)); do
		hi=$((lo + batch - 1))
		[ "$hi" -gt "$KEYS" ] && hi=$KEYS
		cmd='writemode on'
		for ((i = lo; i <= hi; i++)); do cmd="$cmd; set weft/qa/k$(printf '%04d' "$i") v$i"; done
		for attempt in 1 2 3 4 5 6; do
			"$CLI" -C "$CF" --exec "$cmd" --timeout 60 >/dev/null 2>&1 && break
			[ "$attempt" = 6 ] && return 1
		done
	done
	return 0
}

# shellcheck source=wait.sh
. "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)/wait.sh"

rm -rf "$ROOT"
mkdir -p "$ROOT/logs" "$ROOT/d1" "$ROOT/d2" "$ROOT/d3"
printf 'weftqa:weftqa@127.0.0.1:4701,127.0.0.1:4702,127.0.0.1:4703' >"$CF"

for i in 1 2 3; do start "$i"; done
log "redundancy: $REDUNDANCY"
wait_for "configured" 180 cluster_configured
wait_for "available" 180 cluster_answers

log "=== 1. the reported claim ==="
# "Available" is not "fully replicated". The cluster answers long before the second copy of
# every shard exists, so full tolerance is a state to wait for and not a value to sample.
# A fixed sleep hid this: it happened to be long enough, and said nothing about what it was
# waiting for. Removing it turned a hidden race into a named state.
#
# SKIP_CLAIM lets the negative control past this gate, so the kill in stage 3 is the thing
# under test rather than the status line. Never set it for a real run.
if [ "${SKIP_CLAIM:-0}" = 0 ]; then
	wait_for "full fault tolerance" 300 tolerance_is '1 zones'
else
	log "claim check skipped (negative control): $(faulttol)"
fi
STATUS=$("$CLI" -C "$CF" --exec 'status' --timeout 30)
echo "$STATUS" | grep -E 'Coordinators|Fault Tolerance|FoundationDB processes|Zones'
echo "$STATUS" | grep -qE 'Coordinators +- +3' || fail "not three coordinators"

log "=== 2. data goes in ==="
# Many keys, not one. A single key lives on one storage team, so a kill can miss it and
# placement luck then looks like redundancy. The negative control caught exactly that: under
# `single` redundancy, which holds no second copy at all, one key survived a zone loss.
write_keys || fail "write failed after retries"
wait_for "$KEYS keys readable" 120 keys_are "$KEYS"

# The claim is that losing one zone loses nothing. Reading a number that says so is not
# evidence; killing the zone is.
log "=== 3. kill zone z2 ==="
stop "$PID2"
wait_for "available with one zone down" 180 cluster_answers
wait_for "$KEYS keys still readable" 120 keys_are "$KEYS"
log "survived one zone loss. degraded tolerance: $(faulttol)"

log "=== 4. and it heals ==="
start 2
wait_for "available again" 180 cluster_answers
wait_for "tolerance back to 1 zone" 240 tolerance_is '1 zones'

log "=== 5. durability across a full restart ==="
stop "${PIDS[@]}"
PIDS=()
for i in 1 2 3; do start "$i"; done
wait_for "available after a full stop" 240 cluster_answers
wait_for "$KEYS keys survived the restart" 120 keys_are "$KEYS"

log "=== 6. lose consensus: kill two of three ==="
# Every stage above passes when the cluster keeps working. This one passes when it STOPS.
# Three coordinators means a quorum of two. Killing two leaves one, so there is no quorum,
# and FoundationDB must refuse service rather than accept writes it cannot order. A cluster
# that stayed writable here can split-brain, and it would pass every other stage while doing
# it.
stop "$PID2" "$PID3"
wait_for "refusing service without quorum" 180 cluster_refuses
if "$CLI" -C "$CF" --exec 'writemode on; set weft/qa/nope v' --timeout 15 2>&1 | grep -q 'Committed'; then
	fail "committed a write with no quorum"
fi
log "correctly refused the write"

log "=== 7. quorum returns, and so does the data ==="
start 2
start 3
wait_for "available with quorum restored" 240 cluster_answers
wait_for "$KEYS keys after the outage" 120 keys_are "$KEYS"

log "PASS: survived a zone loss, healed, refused service without quorum, and kept its data"
