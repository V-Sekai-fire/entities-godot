#!/bin/bash
# The disaster recovery procedure, run end to end: back up, lose everything, restore.
#
# Losing consensus (qa.sh stage 6) is survivable — quorum comes back and the data is still
# on disk. This is the case where it is not: the cluster and its disks are gone. The only
# thing left is the backup, and the question is whether the procedure actually works.
#
# The destination is file:// here because this machine has no S3 credentials. That is the
# same code path fdbbackup uses for blobstore:// — only the transport differs, so a
# production run swaps the URL and nothing else:
#
#   blobstore://<key>:<secret>@<host>/<name>?bucket=<bucket>
#
# The load-bearing step is 4. Wiping the data directories and proving the keys are *gone*
# before restoring is what stops this passing without the restore doing anything — the same
# anti-vacuity guard ci.yml puts on its crash points.
set -uo pipefail

SERVER=${SERVER:-fdbserver}
CLI=${CLI:-fdbcli}
BACKUP=${BACKUP:-fdbbackup}
RESTORE=${RESTORE:-fdbrestore}
AGENT=${AGENT:-backup_agent}
ROOT=${ROOT:-$HOME/.local/state/weft-fdb-qa/dr}
DEST=${DEST:-file://$ROOT/backup/}
KEYS=${KEYS:-200}
CF="$ROOT/fdb.cluster"
PIDS=()
AGENT_PID=""

log() { echo "[dr] $*"; }
fail() {
	echo "[dr] FAIL: $*" >&2
	exit 1
}
cleanup() {
	log "cleaning up"
	[ -n "$AGENT_PID" ] && kill -9 "$AGENT_PID" 2>/dev/null
	for p in "${PIDS[@]:-}"; do kill -9 "$p" 2>/dev/null || true; done
	wait 2>/dev/null || true
}
trap cleanup EXIT

# shellcheck source=wait.sh
. "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)/wait.sh"

start() {
	local i=$1 port=$((4710 + $1))
	"$SERVER" -p "127.0.0.1:$port" -C "$CF" -d "$ROOT/d$i" -L "$ROOT/logs" \
		--locality-machineid "m$i" --locality-zoneid "z$i" >/dev/null 2>&1 &
	PIDS+=("$!")
}
# States live in wait.sh. There are no sleeps here; see that file for why.
count_keys() {
	# fdbcli opens its quotes with a backtick: `weft/dr/k0001' is `v1'
	"$CLI" -C "$CF" --exec "getrange weft/dr/ weft/dr0 $((KEYS + 10))" --timeout 60 2>/dev/null |
		grep -c "weft/dr/k[0-9]"
}
bring_up() {
	for i in 1 2 3; do start "$i"; done
	wait_for "configured" 180 cluster_configured
	wait_for "available" 180 cluster_answers
}

# Every binary is resolved before anything starts, because the alternative is not a
# failure. `backup_agent` is installed to /usr/lib/foundationdb/backup_agent/ and is not on
# PATH, so the default resolved to nothing, the agent never ran, and `fdbbackup start -w`
# blocked forever waiting for a job nobody would pick up. The run sat silent until the
# unit's TimeoutStartSec killed it -- ten minutes that read as slow rather than broken.
#
# A missing binary is a FAIL here, named, before a cluster exists to clean up.
for tool in SERVER CLI BACKUP RESTORE AGENT; do
	eval "path=\$$tool"
	command -v "$path" >/dev/null 2>&1 || [ -x "$path" ] ||
		fail "$tool: '$path' is not executable and not on PATH"
done

rm -rf "$ROOT"
mkdir -p "$ROOT/logs" "$ROOT/backup" "$ROOT/d1" "$ROOT/d2" "$ROOT/d3"
printf 'weftdr:weftdr@127.0.0.1:4711,127.0.0.1:4712,127.0.0.1:4713' >"$CF"

# 200 sets in one transaction on a freshly configured cluster returns
# commit_unknown_result (1021): data distribution is still settling, and the whole batch is
# one commit. Smaller batches, and a retry, because 1021 means exactly what it says — the
# commit may have succeeded, so retrying a `set` of the same value is safe and idempotent.
write_keys() {
	local batch=50 lo hi cmd attempt
	for ((lo = 1; lo <= KEYS; lo += batch)); do
		hi=$((lo + batch - 1))
		[ "$hi" -gt "$KEYS" ] && hi=$KEYS
		cmd='writemode on'
		for ((i = lo; i <= hi; i++)); do cmd="$cmd; set weft/dr/k$(printf '%04d' "$i") v$i"; done
		# The retry is the wait: fdbcli blocks for its own --timeout, so a failed attempt
		# has already paced the next one. No sleep needed between them.
		for attempt in 1 2 3 4 5 6; do
			"$CLI" -C "$CF" --exec "$cmd" --timeout 60 >/dev/null 2>&1 && break
			[ "$attempt" = 6 ] && return 1
		done
	done
	return 0
}

log "=== 1. a cluster with data in it ==="
bring_up
write_keys || fail "write failed after retries"
wait_for "$KEYS keys readable" 120 keys_are "$KEYS"

log "=== 2. back it up to $DEST ==="
"$AGENT" -C "$CF" >"$ROOT/logs/agent.log" 2>&1 &
AGENT_PID=$!
# `fdbbackup start -w` blocks until the agent picks the job up and finishes it, so the agent
# starting is not a state this has to wait for separately.
"$BACKUP" start -C "$CF" -d "$DEST" -w 2>&1 | tail -3 || fail "backup did not complete"

# `-d file:///base/` is where backups are *kept*; the restorable thing is the container
# underneath it, `.../backup-<timestamp>`. Point a restore at the base and it reads
# `Restorable: false`, `SnapshotBytes: 0` — and says so without failing. A runbook that
# records the base URL looks like it has backups and finds out otherwise at recovery time.
#
# `fdbbackup list` is used rather than ls because it works for blobstore:// too, which is
# what a production run would use.
CONTAINER=$("$BACKUP" list -b "$DEST" 2>/dev/null | grep -oE '(file|blobstore)://[^ ]+' | tail -1)
[ -n "$CONTAINER" ] || fail "no backup container found under $DEST"
log "container: $CONTAINER"

# The check the base URL would have failed. Never trust a backup that has not said this.
DESC=$("$BACKUP" describe -C "$CF" -d "$CONTAINER" 2>&1)
echo "$DESC" | grep -E "Restorable|SnapshotBytes"
echo "$DESC" | grep -q "Restorable: true" || fail "backup is not restorable"
log "backup complete and restorable"

log "=== 3. lose everything ==="
kill -9 "$AGENT_PID" 2>/dev/null
AGENT_PID=""
for p in "${PIDS[@]}"; do kill -9 "$p" 2>/dev/null || true; done
PIDS=()
rm -rf "$ROOT/d1" "$ROOT/d2" "$ROOT/d3"
mkdir -p "$ROOT/d1" "$ROOT/d2" "$ROOT/d3"
log "data directories wiped"

log "=== 4. prove the loss is real ==="
bring_up
wait_for "an empty cluster" 120 keys_are 0
log "confirmed empty — the restore below now proves something"

log "=== 5. restore ==="
"$AGENT" -C "$CF" >>"$ROOT/logs/agent.log" 2>&1 &
AGENT_PID=$!
"$RESTORE" start -r "$CONTAINER" --dest-cluster-file "$CF" -w 2>&1 | tail -3 || fail "restore command failed"

log "=== 6. the data is back ==="
wait_for "$KEYS keys restored" 180 keys_are "$KEYS"
got=$(count_keys)
"$CLI" -C "$CF" --exec 'get weft/dr/k0001' --timeout 30 2>&1 | grep -q 'v1' ||
	fail "a restored key has the wrong value"
log "PASS: $got keys backed up, lost with the disks, and restored"
