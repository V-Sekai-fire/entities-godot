#!/bin/sh
# Assert the backup is fresh, from the one place that already holds the
# credentials: the cluster machine. CI has no production access and the spend
# keeper's authority should not grow a monitoring pattern, so the check runs
# beside the agents and publishes a health file that Fly's machine check
# reads. A failing check is visible in `fly status`; nothing new gets a
# secret.
#
# The signal is `fdbbackup status -t <tag>`: whether the tag is restorable,
# and how old its newest complete log is. The cluster-wide `status json`
# fields cannot tell tags apart, so a probe reading them passes the default
# tag's check on the DR tag's data, with no default backup at all.
#
#   backup-fresh.sh              loop forever, refresh /run/backup-fresh/health
#   backup-fresh.sh --tag dr     the same for tag dr, refreshing /run/backup-fresh/dr
#   backup-fresh.sh --self-test
set -eu

MAX_BEHIND=${WEFT_BACKUP_MAX_BEHIND:-3600}
CHECK_EVERY=${WEFT_BACKUP_CHECK_EVERY:-300}
HEALTH_DIR=/run/backup-fresh
VERIFY="Check.Valid=1,S.CN>=fdb-,S.CN<=.chibifire.com"

# verdict <running true|false> <seconds_behind>: fresh only when a backup is
# running AND its restorable point is close. Either signal missing is stale;
# an unreadable status must never read as fresh.
verdict() {
	[ "$1" = "true" ] || { echo stale; return; }
	behind=${2%%.*}
	case "$behind" in *[!0-9]*|"") echo stale; return;; esac
	if [ "$behind" -le "$MAX_BEHIND" ]; then echo fresh; else echo stale; fi
}

# parse <fdbbackup status text> <now as POSIX time>: prints "<running> <seconds_behind>",
# x for a lag it cannot read. A tag mid-initial-snapshot has no complete log yet and
# reads 0 behind, so a first rollout does not stall the deploy gate.
parse() {
	case "$1" in
		*"submitted for termination"*|*terminated*|*aborted*|*"No previous backups"*) running=false ;;
		*restorable*|*"is in progress"*|*submitted*) running=true ;;
		*) running=false ;;
	esac
	ts=$(printf '%s' "$1" | grep -oE 'Last complete log version and timestamp[^,]*, [0-9/.:+-]+' | awk -F', ' '{print $2}' | head -1)
	behind=x
	if [ -n "$ts" ]; then
		epoch=$(date -u -d "$(printf '%s' "$ts" | sed 's|/|-|g; s|\.| |')" +%s 2>/dev/null || echo "")
		[ -z "$epoch" ] || behind=$(( $2 - epoch ))
	elif [ "$running" = true ]; then
		behind=0
	fi
	echo "$running $behind"
}

status_text() {
	now=$(date -u +%s)
	at=$(date -u -d "@$((now - $1))" +%Y/%m/%d.%H:%M:%S+0000)
	printf "The backup on tag \`t' is restorable but continuing to blobstore://k@h:8444/weft?bucket=b.\n"
	printf ' Last complete log version and timestamp        - 2711158840524, %s\n' "$at"
}

self_test() {
	# Both directions, or the loop is not armed: a planted defect the check
	# cannot flag is a check that certifies the defect.
	now=$(date -u +%s)
	[ "$(verdict true 12.5)" = fresh ] || { echo "FAIL control: fresh read as stale" >&2; return 1; }
	[ "$(verdict true $((MAX_BEHIND + 100)))" = stale ] || { echo "FAIL control: planted lag read as fresh" >&2; return 1; }
	[ "$(verdict false 12.5)" = stale ] || { echo "FAIL control: stopped backup read as fresh" >&2; return 1; }
	[ "$(verdict true garbage)" = stale ] || { echo "FAIL control: unreadable lag read as fresh" >&2; return 1; }
	# shellcheck disable=SC2046
	[ "$(verdict $(parse "$(status_text 60)" "$now"))" = fresh ] || { echo "FAIL control: a restorable tag with a log 60 s old read as stale" >&2; return 1; }
	# shellcheck disable=SC2046
	[ "$(verdict $(parse "$(status_text $((MAX_BEHIND + 600)))" "$now"))" = stale ] || { echo "FAIL control: a newest log older than the threshold read as fresh" >&2; return 1; }
	# shellcheck disable=SC2046
	[ "$(verdict $(parse "No previous backups found." "$now"))" = stale ] || { echo "FAIL control: a tag that does not exist read as fresh" >&2; return 1; }
	# shellcheck disable=SC2046
	[ "$(verdict $(parse "The backup on tag \`t' has been submitted for termination." "$now"))" = stale ] || { echo "FAIL control: a terminating tag read as fresh" >&2; return 1; }
	echo "ok   8 of 8 controls fired"
}

TAG=""
if [ "${1:-}" = "--tag" ]; then
	TAG="${2:?--tag needs a value}"
fi

if [ "${1:-}" = "--self-test" ]; then
	self_test
	exit $?
fi

self_test || exit 1
mkdir -p "$HEALTH_DIR"

# The default tag writes /health, which fly.toml [checks.backup_fresh] polls. A named
# tag writes /$TAG, and fly.toml adds one [checks.backup_fresh_$TAG] per tag it gates.
HEALTH_FILE="$HEALTH_DIR/${TAG:-health}"

while :; do
	out=$(timeout 30 fdbbackup status -t "${TAG:-default}" \
		--tls_certificate_file /etc/foundationdb/tls/cert.pem \
		--tls_key_file /etc/foundationdb/tls/key.pem \
		--tls_ca_file /etc/foundationdb/tls/ca.pem \
		--tls_verify_peers "$VERIFY" 2>/dev/null || true)
	# shellcheck disable=SC2046
	set -- $(parse "$out" "$(date -u +%s)")
	if [ "$(verdict "$1" "$2")" = fresh ]; then
		echo ok > "$HEALTH_FILE"
	else
		rm -f "$HEALTH_FILE"
		echo "[backup-fresh ${TAG:-default}] BACKUP STALE: running=$1 seconds_behind=$2 max=${MAX_BEHIND}" >&2
	fi
	sleep "$CHECK_EVERY"
done
