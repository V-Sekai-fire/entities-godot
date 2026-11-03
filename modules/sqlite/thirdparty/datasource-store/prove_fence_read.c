// Prove that a handle whose view of the store is stale is refused a read.
//
//   prove_fence_read writer <db>            a second writer takes the fence; the first one's read fails
//   prove_fence_read reader <db>            a writer commits; a NORMAL-locking reader follows the head
//   prove_fence_read reader-exclusive <db>  the same under EXCLUSIVE locking: refused until reopened
//
// A handle caches pages, and the fence is what keeps that cache true. Before `check_view`
// the fence covered every write transaction and no read transaction, so a handle that had
// lost the fence kept reading pages from a store another writer now owned, beside the pages
// it had cached before. `spec/ViewCoherence.lean` is the argument; this is the measurement.
//
// `writer` keeps the first handle's page cache too small to hold the table, so its read has
// to reach the store, which is where the check runs. A read SQLite answers from its own cache
// never reaches the VFS, and the comment on `check_view` says why that is out of scope. The
// refused write comes first because it ends the handle's cached read transaction; without
// it the handle reads its last snapshot, coherent and stale, for up to four seconds.
//
// `reader` opens read-only under NORMAL locking, so SQLite reads page 1 at every transaction,
// sees the head as its change counter, drops its cache and reads the commit. `reader-exclusive`
// never re-reads page 1, so the only safe answer once the head moved is a refusal, and a
// reopened handle reads the new head.

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int weft_fdb_start(const char *cluster_file);
void weft_fdb_stop(void);
int weft_vfs_register(int make_default);

static int fail(sqlite3 *db, const char *what) {
	fprintf(stderr, "%s: %s\n", what, db ? sqlite3_errmsg(db) : "no handle");
	return 1;
}

static int run(sqlite3 *db, const char *sql) {
	char *err = NULL;
	if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
		fprintf(stderr, "%s -> %s\n", sql, err ? err : "?");
		sqlite3_free(err);
		return 1;
	}
	return 0;
}

// Count the rows of kv. Returns the step result, and the count through `out` on SQLITE_ROW.
static int count(sqlite3 *db, long long *out) {
	sqlite3_stmt *st = NULL;
	int rc = sqlite3_prepare_v2(db, "SELECT count(*) FROM kv", -1, &st, NULL);
	if (rc != SQLITE_OK) return rc;
	rc = sqlite3_step(st);
	if (rc == SQLITE_ROW) *out = sqlite3_column_int64(st, 0);
	sqlite3_finalize(st);
	return rc;
}

static int open_db(const char *name, int flags, sqlite3 **db) {
	if (sqlite3_open_v2(name, db, flags, "weft_fdb")) return fail(*db, "open");
	if (run(*db, "PRAGMA journal_mode=MEMORY")) return 1;
	return 0;
}

static int fill(sqlite3 *db, int rows) {
	if (run(db, "CREATE TABLE IF NOT EXISTS kv (k INTEGER PRIMARY KEY, v TEXT)")) return 1;
	if (run(db, "DELETE FROM kv")) return 1;
	if (run(db, "BEGIN")) return 1;
	sqlite3_stmt *st = NULL;
	if (sqlite3_prepare_v2(db, "INSERT INTO kv VALUES (?, ?)", -1, &st, NULL))
		return fail(db, "prepare insert");
	char v[1024];
	memset(v, 'v', sizeof v - 1);
	v[sizeof v - 1] = 0;
	for (int i = 0; i < rows; i++) {
		sqlite3_bind_int(st, 1, i);
		sqlite3_bind_text(st, 2, v, -1, SQLITE_STATIC);
		if (sqlite3_step(st) != SQLITE_DONE) return fail(db, "insert");
		sqlite3_reset(st);
	}
	sqlite3_finalize(st);
	return run(db, "COMMIT");
}

static const int ROWS = 2000; // about 500 pages of 4 KiB, against a cache of 16

static int writer(const char *name) {
	sqlite3 *a = NULL, *b = NULL, *c = NULL;
	long long n = 0;

	if (open_db(name, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, &a)) return 1;
	if (run(a, "PRAGMA locking_mode=EXCLUSIVE")) return 1;
	if (run(a, "PRAGMA cache_size=16")) return 1;
	if (fill(a, ROWS)) return 1;
	if (count(a, &n) != SQLITE_ROW || n != ROWS) return fail(a, "first count");
	printf("a wrote and read %lld rows\n", n);

	// A second writer takes the fence and changes the store.
	if (open_db(name, SQLITE_OPEN_READWRITE, &b)) return 1;
	if (run(b, "PRAGMA locking_mode=EXCLUSIVE")) return 1;
	if (run(b, "DELETE FROM kv WHERE k % 2 = 0")) return 1;
	if (run(b, "INSERT INTO kv SELECT k + 100000, v FROM kv")) return 1;
	if (count(b, &n) != SQLITE_ROW || n != ROWS) return fail(b, "b count");
	printf("b took the fence and committed; b reads %lld rows\n", n);

	// The stale handle's write is refused, which is what the fence always did.
	int rc = sqlite3_exec(a, "INSERT INTO kv VALUES (-1, 'stale')", NULL, NULL, NULL);
	printf("a's write after losing the fence: rc=%d (%s)\n", rc, sqlite3_errmsg(a));
	if ((rc & 0xff) != SQLITE_READONLY) {
		fprintf(stderr, "expected SQLITE_READONLY for the stale write, got %d\n", rc);
		return 1;
	}

	// The stale handle's read must reach the store and be refused there.
	rc = count(a, &n);
	if (rc == SQLITE_ROW) {
		fprintf(stderr, "a read %lld rows through a lost fence; it should have been refused\n", n);
		return 1;
	}
	int ext = sqlite3_extended_errcode(a);
	printf("a's read after losing the fence: rc=%d extended=%d (%s)\n", rc, ext, sqlite3_errmsg(a));
	if ((rc & 0xff) != SQLITE_IOERR) {
		fprintf(stderr, "expected SQLITE_IOERR, got %d\n", rc);
		return 1;
	}
	// And it stays refused: the database it was reading is not there any more.
	if (count(a, &n) == SQLITE_ROW) {
		fprintf(stderr, "a read %lld rows on the second try\n", n);
		return 1;
	}

	// A fresh handle reads what b left.
	if (open_db(name, SQLITE_OPEN_READWRITE, &c)) return 1;
	if (count(c, &n) != SQLITE_ROW || n != ROWS) return fail(c, "c count");
	printf("a fresh handle reads %lld rows\n", n);

	sqlite3_close(c);
	sqlite3_close(b);
	sqlite3_close(a);
	return 0;
}

static int reader(const char *name) {
	sqlite3 *w = NULL, *r = NULL;
	long long n = 0, m = 0;

	if (open_db(name, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, &w)) return 1;
	if (run(w, "PRAGMA locking_mode=EXCLUSIVE")) return 1;
	if (fill(w, 50)) return 1;

	if (open_db(name, SQLITE_OPEN_READONLY, &r)) return 1;
	if (count(r, &n) != SQLITE_ROW || n != 50) return fail(r, "reader count");
	printf("reader sees %lld rows\n", n);

	if (run(w, "INSERT INTO kv VALUES (1000, 'late')")) return 1;
	printf("writer committed one more row\n");

	// Under NORMAL locking SQLite re-reads the header at the next transaction, the
	// reader's change counter is the head, and the cache goes. The read follows.
	if (count(r, &m) != SQLITE_ROW) return fail(r, "reader follow");
	printf("reader's next read sees %lld rows\n", m);
	if (m != 51) {
		fprintf(stderr, "expected 51 rows after following the head, got %lld\n", m);
		return 1;
	}

	sqlite3_close(r);
	sqlite3_close(w);
	return 0;
}

static int reader_exclusive(const char *name) {
	sqlite3 *w = NULL, *r = NULL;
	long long n = 0, m = 0;

	if (open_db(name, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, &w)) return 1;
	if (run(w, "PRAGMA locking_mode=EXCLUSIVE")) return 1;
	if (fill(w, ROWS)) return 1;

	// The table outgrows the cache, as in `writer`, so the count has to reach the store.
	if (open_db(name, SQLITE_OPEN_READONLY, &r)) return 1;
	if (run(r, "PRAGMA locking_mode=EXCLUSIVE")) return 1;
	if (run(r, "PRAGMA cache_size=16")) return 1;
	if (count(r, &n) != SQLITE_ROW || n != ROWS) return fail(r, "reader count");
	printf("exclusive reader sees %lld rows\n", n);

	if (run(w, "INSERT INTO kv SELECT k + 100000, v FROM kv")) return 1;
	printf("writer committed %d more rows\n", ROWS);

	// An EXCLUSIVE reader never re-reads the header, so SQLite would use its cache beside
	// the new pages. Its cached read transaction lives four seconds; after that, refused.
	sqlite3_sleep(4500);
	int rc = count(r, &m);
	if (rc == SQLITE_ROW) {
		fprintf(stderr, "exclusive reader answered %lld rows across a moved head\n", m);
		return 1;
	}
	printf("exclusive reader's read after the commit: rc=%d (%s)\n", rc, sqlite3_errmsg(r));
	if ((rc & 0xff) != SQLITE_IOERR) {
		fprintf(stderr, "expected SQLITE_IOERR, got %d\n", rc);
		return 1;
	}
	if (count(r, &m) == SQLITE_ROW) {
		fprintf(stderr, "exclusive reader answered %lld rows on the second try\n", m);
		return 1;
	}
	sqlite3_close(r);

	// Reopened, it reads the new head.
	if (open_db(name, SQLITE_OPEN_READONLY, &r)) return 1;
	if (count(r, &m) != SQLITE_ROW || m != 2 * ROWS) return fail(r, "reopened reader");
	printf("reopened reader sees %lld rows\n", m);

	sqlite3_close(r);
	sqlite3_close(w);
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: prove_fence_read writer|reader|reader-exclusive <db>\n");
		return 2;
	}
	int err = weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"));
	if (err) {
		fprintf(stderr, "FoundationDB did not start: %d\n", err);
		return 1;
	}
	weft_vfs_register(1);

	int rc;
	if (strcmp(argv[1], "writer") == 0) rc = writer(argv[2]);
	else if (strcmp(argv[1], "reader") == 0) rc = reader(argv[2]);
	else if (strcmp(argv[1], "reader-exclusive") == 0) rc = reader_exclusive(argv[2]);
	else {
		fprintf(stderr, "unknown mode %s\n", argv[1]);
		rc = 2;
	}
	weft_fdb_stop();
	return rc;
}
