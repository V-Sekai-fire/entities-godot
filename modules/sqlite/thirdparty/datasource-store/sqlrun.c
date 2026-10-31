// Run arbitrary SQL against a database that lives in FoundationDB.
//
//   sqlrun <db> < query.sql
//   echo 'select count(*) from orders' | sqlrun tpcc.db
//   sqlrun --readonly <db> < analytics.sql
//
// Every other program here asserts one property and exits. This one asserts nothing: it
// reads SQL on stdin, runs it through the VFS, and prints what came back with the time it
// took. That makes it a tool rather than a test, which is why it is not in the default
// build -- a program with no assertion cannot fail a commit stage, so building it by
// default only slows down the build that can.
//
// It exists because the analytical half of the workload had no way to be run at all. The
// benchmarks drive fixed statements compiled into them, and `integrity` runs one pragma.
// Neither can answer "what does a five-table aggregate cost when the pages are 300 km
// away", and that question is the one an OLAP claim rests on.
//
// Timing note: the elapsed time reported for each statement is wall clock around
// prepare/step/finalize, so it includes the FoundationDB round trips that are the whole
// point of measuring it. It does not subtract the time to print rows, so a query that
// returns many rows to a terminal measures the terminal too. Use `--quiet` when the number
// is what matters.

#define _POSIX_C_SOURCE 200809L

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int weft_fdb_start(const char *cluster_file);
void weft_fdb_stop(void);
int weft_vfs_register(int make_default);

static double now_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(int argc, char **argv) {
	int readonly = 0, quiet = 0;
	const char *path = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--readonly") == 0) readonly = 1;
		else if (strcmp(argv[i], "--quiet") == 0) quiet = 1;
		else path = argv[i];
	}
	if (!path) {
		fprintf(stderr, "usage: sqlrun [--readonly] [--quiet] <db> < statements.sql\n");
		return 2;
	}

	// Read all of stdin. SQL is small and the alternative is a line-oriented parser that
	// gets multi-line statements wrong.
	size_t cap = 1 << 16, len = 0;
	char *sql = malloc(cap);
	if (!sql) return 1;
	for (;;) {
		if (len + 4096 > cap) {
			cap *= 2;
			char *bigger = realloc(sql, cap);
			if (!bigger) { free(sql); return 1; }
			sql = bigger;
		}
		size_t n = fread(sql + len, 1, 4096, stdin);
		len += n;
		if (n < 4096) break;
	}
	sql[len] = '\0';

	if (weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"))) {
		fprintf(stderr, "FoundationDB did not start\n");
		free(sql);
		return 1;
	}
	weft_vfs_register(0);

	sqlite3 *db = NULL;
	int flags = readonly ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
	if (sqlite3_open_v2(path, &db, flags, "weft_fdb")) {
		fprintf(stderr, "open: %s\n", sqlite3_errmsg(db));
		free(sql);
		return 1;
	}

	int rc = 0;
	const char *tail = sql;
	while (tail && *tail) {
		sqlite3_stmt *st = NULL;
		const char *next = NULL;
		if (sqlite3_prepare_v2(db, tail, -1, &st, &next)) {
			fprintf(stderr, "prepare: %s\n", sqlite3_errmsg(db));
			rc = 1;
			break;
		}
		if (!st) { tail = next; continue; }  // whitespace or a comment

		double t0 = now_ms();
		int cols = sqlite3_column_count(st), rows = 0, step;

		while ((step = sqlite3_step(st)) == SQLITE_ROW) {
			rows++;
			if (quiet) continue;
			for (int c = 0; c < cols; c++) {
				const unsigned char *v = sqlite3_column_text(st, c);
				printf("%s%s", v ? (const char *)v : "", c + 1 < cols ? "\t" : "\n");
			}
		}
		double ms = now_ms() - t0;

		if (step != SQLITE_DONE) {
			fprintf(stderr, "step: %s\n", sqlite3_errmsg(db));
			rc = 1;
			sqlite3_finalize(st);
			break;
		}
		fprintf(stderr, "-- %d row(s), %.1f ms\n", rows, ms);
		sqlite3_finalize(st);
		tail = next;
	}

	sqlite3_close(db);
	weft_fdb_stop();
	free(sql);
	return rc;
}
