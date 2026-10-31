// A grant across two databases is never partial.
//
//   prove_parallel_commit <world> <avatar> <grants> at <n>
//   prove_parallel_commit <world> <avatar> <grants> check
//
// One item leaves the world and reaches an avatar. Those are two databases, so they are
// two `flush` calls and two FoundationDB transactions, and nothing in the VFS made them
// one event until the parallel commit protocol went in. This looks for the gap.
//
// The invariant is conservation, and it is the whole test. Every item is either in the
// world or held by the avatar, and never both and never neither. So:
//
//     world.remaining + avatar.held = grants
//
// A crash that lands between the two commits breaks that sum by one, in whichever
// direction the crash fell. `PRAGMA integrity_check` cannot see it: both databases are
// structurally perfect, and they simply disagree about where an item went. This is the
// same blind spot `prove_crash` exists for, one level up: there the two halves of one
// commit, here two commits that have to be one.
//
// `at <n>` stops the process before the Nth write transaction, the repeatable crash that
// `prove_crash` uses, so a search can address a failure it finds. The interesting crash
// points are the two the protocol is built around: between staging the intents and
// writing the record, where the group must abort; and between the record and moving the
// heads, where the group is already implicitly committed and must be finished by whoever
// finds it.
//
// `check` runs recovery and then tests the sum. It is a separate mode because recovery is
// the thing under test: a crashed grant is only decided when somebody else looks.
//
// Exit: 0 the sum holds, 1 it does not, 2 the crash point was never reached.

#define _POSIX_C_SOURCE 200809L

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int weft_fdb_start(const char *cluster_file);
void weft_fdb_stop(void);
int weft_vfs_register(int make_default);
int weft_txn_begin(unsigned long long *txnid);
int weft_txn_join(sqlite3 *db, unsigned long long txnid);
int weft_txn_commit(unsigned long long txnid);
int weft_txn_abort(unsigned long long txnid);

static int run(sqlite3 *db, const char *sql) {
	char *err = NULL;
	if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
		fprintf(stderr, "%s -> %s\n", sql, err ? err : "?");
		sqlite3_free(err);
		return 1;
	}
	return 0;
}

static sqlite3 *open_db(const char *name) {
	sqlite3 *db = NULL;
	if (sqlite3_open_v2(name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, "weft_fdb")) {
		fprintf(stderr, "open %s: %s\n", name, db ? sqlite3_errmsg(db) : "no handle");
		return NULL;
	}
	// The journal has to stay in memory, or SQLite writes one behind the VFS and the
	// staged pages are no longer the whole of what a participant wrote.
	if (run(db, "PRAGMA journal_mode=MEMORY")) return NULL;
	if (run(db, "PRAGMA locking_mode=EXCLUSIVE")) return NULL;
	return db;
}

static int count_of(sqlite3 *db, const char *sql) {
	sqlite3_stmt *st;
	if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) return -1;
	int n = (sqlite3_step(st) == SQLITE_ROW) ? sqlite3_column_int(st, 0) : -1;
	sqlite3_finalize(st);
	return n;
}

// A group that is begun and given up must leave nothing behind.
//
// A participant stages under head + 1, and a group that never committed moved no head, so
// the next group stages under the same txid. If the abandoned pages survive, they answer
// the next group's intent check and two groups' writes commit as one. This gives up a group
// deliberately, then makes a real grant, and the conservation check at the end is what says
// whether the dead group's pages came along.
static int abandon_a_group(sqlite3 *world, sqlite3 *avatar) {
	char sql[160];
	unsigned long long txnid = 0;

	if (weft_txn_begin(&txnid) != SQLITE_OK) return 1;
	if (weft_txn_join(world, txnid) != SQLITE_OK) goto give_up;
	if (weft_txn_join(avatar, txnid) != SQLITE_OK) goto give_up;

	// A group is an explicit SQLite transaction, and giving one up is a ROLLBACK.
	//
	// Clearing the staged pages is not enough on its own. SQLite has a page cache of its
	// own, and with locking_mode=EXCLUSIVE it trusts it: a statement that ran in autocommit
	// has already finished as far as SQLite is concerned, and nothing the VFS does to
	// FoundationDB afterwards can make it forget. The next commit would carry those pages
	// forward and the abandoned write would arrive under a later group.
	//
	// Inside a transaction there is nothing to unwind, because SQLite writes its pages at
	// COMMIT and a ROLLBACK never reaches the VFS at all.
	if (run(avatar, "BEGIN")) goto give_up;
	snprintf(sql, sizeof sql, "INSERT OR REPLACE INTO held VALUES (%d)", 9999);
	if (run(avatar, sql)) { sqlite3_exec(avatar, "ROLLBACK", NULL, NULL, NULL); goto give_up; }
	if (run(avatar, "ROLLBACK")) goto give_up;

give_up:
	return weft_txn_abort(txnid) != SQLITE_OK;
}

// Move one item from the world to the avatar, as one group.
//
// Both databases join the group before either is written to. That order is the whole
// point: SQLite ends a statement by calling xSync, and a file in a group stages there
// instead of committing, so neither write is visible on its own. Joining afterwards would
// be too late, because the first statement would already have committed by itself.
//
// The record is then written in one transaction, and that write is the commit: after it
// the grant has happened, whether or not this process lives to move the heads.
static int grant(sqlite3 *world, sqlite3 *avatar, int item) {
	char sql[160];
	unsigned long long txnid = 0;

	if (weft_txn_begin(&txnid) != SQLITE_OK) return 1;

	if (weft_txn_join(world, txnid) != SQLITE_OK) goto give_up;
	if (weft_txn_join(avatar, txnid) != SQLITE_OK) goto give_up;

	// One explicit transaction for each participant, so each stages once at its COMMIT
	// rather than once per statement, and so a failure part way can be rolled back.
	if (run(world, "BEGIN")) goto give_up;
	if (run(avatar, "BEGIN")) goto give_up;

	snprintf(sql, sizeof sql, "DELETE FROM loot WHERE k = %d", item);
	if (run(world, sql)) goto give_up;
	snprintf(sql, sizeof sql, "INSERT OR REPLACE INTO held VALUES (%d)", item);
	if (run(avatar, sql)) goto give_up;

	// These COMMITs stage; they do not commit. The record does that.
	if (run(world, "COMMIT")) goto give_up;
	if (run(avatar, "COMMIT")) goto give_up;

	return weft_txn_commit(txnid) != SQLITE_OK;

give_up:
	// The group holds a lock until it is committed or given up, so an error here has to
	// let go of it rather than leave the next grant waiting on a group that is over.
	weft_txn_abort(txnid);
	return 1;
}

static int writer(const char *world_name, const char *avatar_name, int grants) {
	if (weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"))) return 1;
	weft_vfs_register(1);

	sqlite3 *world = open_db(world_name);
	sqlite3 *avatar = open_db(avatar_name);
	if (!world || !avatar) return 1;

	if (run(world, "DROP TABLE IF EXISTS loot")) return 1;
	if (run(world, "CREATE TABLE loot (k INTEGER PRIMARY KEY)")) return 1;
	if (run(avatar, "DROP TABLE IF EXISTS held")) return 1;
	if (run(avatar, "CREATE TABLE held (k INTEGER PRIMARY KEY)")) return 1;

	char sql[128];
	if (run(world, "BEGIN")) return 1;
	for (int i = 0; i < grants; i++) {
		snprintf(sql, sizeof sql, "INSERT INTO loot VALUES (%d)", i);
		if (run(world, sql)) return 1;
	}
	if (run(world, "COMMIT")) return 1;

	// Give up a group before handing anything over, so the grants below stage under a txid
	// an abandoned group already used.
	if (abandon_a_group(world, avatar)) return 1;

	// Hand every item over, one group at a time. The crash lands in one of them.
	for (int i = 0; i < grants; i++) {
		if (grant(world, avatar, i)) return 1;
	}

	sqlite3_close(world);
	sqlite3_close(avatar);
	weft_fdb_stop();
	return 0;
}

// Decide whatever the crash left, then test conservation.
static int check(const char *world_name, const char *avatar_name, int grants) {
	if (weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"))) return 1;
	weft_vfs_register(1);

	// Nothing calls recovery here, and that is the point. Opening raises a fence, and a
	// fence raised over a group that was already implicitly committed would prevent a commit
	// that has happened, so recovery has to come first. `vfs_open` does it, so the ordering
	// holds whether or not a caller knows about it. If that ever stops being true, the sum
	// below is what says so.
	sqlite3 *world = open_db(world_name);
	sqlite3 *avatar = open_db(avatar_name);
	if (!world || !avatar) return 1;

	const int remaining = count_of(world, "SELECT count(*) FROM loot");
	const int held = count_of(avatar, "SELECT count(*) FROM held");

	// The abandoned group wrote key 9999 and was given up. If it is here, a dead group's
	// pages were committed by a later group.
	const int ghost = count_of(avatar, "SELECT count(*) FROM held WHERE k = 9999");
	if (ghost > 0) {
		printf("GHOST: a group that was given up left %d row(s) behind\n", ghost);
	}
	if (remaining < 0 || held < 0) {
		printf("no tables yet, so nothing was granted\n");
		sqlite3_close(world);
		sqlite3_close(avatar);
		weft_fdb_stop();
		return 0;
	}

	printf("world %d + avatar %d = %d, expected %d\n", remaining, held, remaining + held,
	       grants);

	// Conservation only describes a seeded world. A crash during setup leaves the tables
	// made and the seed not landed, and then there is nothing to conserve: the sum is zero
	// because no item ever existed, which is not a partial grant.
	//
	// A half seeded world cannot happen, because the seed is one SQLite commit and
	// `prove_crash` is the program that establishes those land whole or not at all. So the
	// sum is zero or it is `grants`, and any other value is an item that went missing or
	// got duplicated between the two databases.
	int bad = ghost > 0;
	if (remaining + held == 0) {
		printf("unseeded: the crash landed in setup, so there was nothing to conserve\n");
	} else if (remaining + held != grants) {
		// One item is in both places or in neither, which is a grant that half happened.
		printf("PARTIAL: %d items accounted for, expected %d\n", remaining + held, grants);
		bad = 1;
	} else {
		printf("conserved: %d items, every grant whole\n", grants);
	}

	// Both databases are structurally fine either way, which is the point of checking the
	// sum rather than asking SQLite.
	sqlite3_stmt *st;
	if (sqlite3_prepare_v2(world, "PRAGMA integrity_check", -1, &st, NULL) == SQLITE_OK) {
		while (sqlite3_step(st) == SQLITE_ROW) {
			const unsigned char *line = sqlite3_column_text(st, 0);
			printf("world integrity_check: %s\n", line ? (const char *)line : "?");
		}
		sqlite3_finalize(st);
	}

	sqlite3_close(world);
	sqlite3_close(avatar);
	weft_fdb_stop();
	return bad;
}

int main(int argc, char **argv) {
	if (argc < 5) {
		fprintf(stderr, "usage: prove_parallel_commit <world> <avatar> <grants> at <n>\n");
		fprintf(stderr, "       prove_parallel_commit <world> <avatar> <grants> check\n");
		return 2;
	}
	const char *world = argv[1];
	const char *avatar = argv[2];
	const int grants = atoi(argv[3]);
	const char *mode = argv[4];

	if (strcmp(mode, "check") == 0) {
		return check(world, avatar, grants);
	}
	if (strcmp(mode, "at") != 0 || argc < 6) {
		fprintf(stderr, "mode must be at <n> or check\n");
		return 2;
	}

	// Fork before the FoundationDB client starts, so no client state crosses it. The child
	// stops itself from the inside, which repeats exactly.
	pid_t child = fork();
	if (child < 0) {
		perror("fork");
		return 1;
	}
	if (child == 0) {
		setenv("WEFT_CRASH_AT_COMMIT", argv[5], 1);
		_exit(writer(world, avatar, grants));
	}

	int status = 0;
	waitpid(child, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		printf("unreached: fewer than %s write transactions in %d grants\n", argv[5], grants);
		return 2;
	}

	return check(world, avatar, grants);
}
