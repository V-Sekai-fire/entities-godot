# Waiting on states, not on the clock.
#
# A fixed wait is a guess. Too short and the test is flaky; too long and every run pays for
# the worst case; and either way the failure says "the next command did not work" instead of
# "the cluster never reached this state". None of the numbers in a `sleep` are observations.
#
# So every wait here names a state and observes a predicate, with a deadline. A timeout
# reports the state that was never reached.
#
# There is no poll interval either. The predicates are `fdbcli` calls with their own
# `--timeout`, so the command blocks and paces the loop. The clock is used to decide when to
# give up, never to decide when to look.
#
# SPDX-License-Identifier: Apache-2.0

# wait_for <state> <deadline-seconds> <predicate...>
wait_for() {
	local state=$1 budget=$2
	shift 2
	local deadline=$(($(date +%s) + budget))
	local tries=0
	while [ "$(date +%s)" -lt "$deadline" ]; do
		tries=$((tries + 1))
		if "$@"; then
			log "reached '$state' after $tries checks"
			return 0
		fi
	done
	fail "never reached '$state' within ${budget}s ($tries checks)"
}

# The observable states of a cluster. Each one blocks inside fdbcli rather than sleeping.

cluster_answers() {
	"$CLI" -C "$CF" --exec 'status minimal' --timeout 10 2>&1 |
		grep -q 'The database is available'
}

cluster_refuses() { ! cluster_answers; }

# `configure new` is the transition into "configured", so retrying it *is* the wait for the
# processes to come up. There is no separate "the servers have started" state to guess at.
cluster_configured() {
	"$CLI" -C "$CF" --exec "configure new ${REDUNDANCY:-double} ssd" --timeout 30 2>&1 |
		grep -qE 'Database created|Database already exists|already exists'
}

tolerance_is() { faulttol | grep -q "$1"; }

keys_are() { [ "$(count_keys)" = "$1" ]; }
