// Prove that the clock works through weft_fdb on a SQLite with no xCurrentTime.
//
//   prove_clock <db>
//
// A SQLite built with SQLITE_OMIT_DEPRECATED, as exqlite builds it, leaves the "unix" VFS's
// xCurrentTime NULL and keeps only xCurrentTimeInt64. weft_fdb took its clock from
// "unix"->xCurrentTime, so the first CURRENT_TIMESTAMP inside the BEAM called address 0.
// This swaps "unix" for a copy without xCurrentTime, the shape that SQLite has, then writes
// a CURRENT_TIMESTAMP row through weft_fdb and checks the time it got.

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int weft_fdb_start(const char *cluster_file);
int weft_vfs_register(int make_default);

static sqlite3_vfs g_unix_copy;

static int run(sqlite3 *db, const char *sql) {
	char *err = NULL;
	if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
		fprintf(stderr, "%s -> %s\n", sql, err ? err : "?");
		sqlite3_free(err);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: prove_clock <db>\n");
		return 2;
	}
	sqlite3_vfs *unix_vfs = sqlite3_vfs_find("unix");
	if (!unix_vfs || unix_vfs->iVersion < 2 || !unix_vfs->xCurrentTimeInt64) {
		fprintf(stderr, "no \"unix\" VFS with xCurrentTimeInt64 to copy\n");
		return 1;
	}
	g_unix_copy = *unix_vfs;
	g_unix_copy.xCurrentTime = NULL;
	if (sqlite3_vfs_unregister(unix_vfs) || sqlite3_vfs_register(&g_unix_copy, 1)) {
		fprintf(stderr, "could not swap in the copy of \"unix\"\n");
		return 1;
	}
	if (sqlite3_vfs_find("unix")->xCurrentTime != NULL || sqlite3_vfs_find(0) != &g_unix_copy) {
		fprintf(stderr, "control failed: \"unix\" still has xCurrentTime, or is not the default\n");
		return 1;
	}
	printf("ok   control: \"unix\" is the default VFS and has no xCurrentTime\n");

	if (weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"))) {
		fprintf(stderr, "FoundationDB did not start\n");
		return 1;
	}
	if (weft_vfs_register(0) != SQLITE_OK) {
		fprintf(stderr, "weft_fdb did not register\n");
		return 1;
	}

	sqlite3 *db = NULL;
	if (sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, "weft_fdb") != SQLITE_OK) {
		fprintf(stderr, "open: %s\n", db ? sqlite3_errmsg(db) : "no handle");
		return 1;
	}
	if (run(db, "PRAGMA journal_mode=MEMORY") || run(db, "PRAGMA locking_mode=EXCLUSIVE") ||
			run(db, "CREATE TABLE IF NOT EXISTS runs (at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP)") ||
			run(db, "DELETE FROM runs") || run(db, "INSERT INTO runs DEFAULT VALUES")) {
		return 1;
	}

	sqlite3_stmt *st = NULL;
	if (sqlite3_prepare_v2(db, "SELECT CAST(strftime('%s', at) AS INTEGER) FROM runs", -1, &st, NULL) != SQLITE_OK ||
			sqlite3_step(st) != SQLITE_ROW) {
		fprintf(stderr, "read back: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	long long at = sqlite3_column_int64(st, 0);
	sqlite3_finalize(st);
	run(db, "DROP TABLE runs");
	sqlite3_close(db);

	long long now = (long long)time(NULL);
	if (at < now - 60 || at > now + 60) {
		fprintf(stderr, "CURRENT_TIMESTAMP through weft_fdb read %lld, the system clock %lld\n", at, now);
		return 1;
	}
	printf("ok   CURRENT_TIMESTAMP through weft_fdb is within a minute of the system clock (%+lld s)\n", at - now);
	return 0;
}
