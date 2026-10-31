// Copyright (c) 2026 K. S. Ernest (iFire) Lee
// SPDX-License-Identifier: MIT
//
// The store helper behind TaskweftAcp.Session.Store.Vfs: one process per open set of
// databases, driven over stdin/stdout by a line protocol, answering JSON. Built from
// fabric-store's sqlrun.c: plain mode opens ordinary SQLite files (a desk, CI); fabric
// mode (WEFT_FABRIC defined at build time) opens weft_fdb databases on the cluster and
// exposes the parallel-commit calls.
//
//   O <name> <base64 path-or-dbname>          open a database under <name>
//   Q <name> <base64 sql> <argc>              run one statement; argc lines follow:
//       i:<int> | f:<float> | s:<base64 text> | b:<base64 blob> | n:
//   B                                          weft_txn_begin       (fabric)
//   J <name> <txnid>                           weft_txn_join        (fabric)
//   C <txnid> | A <txnid>                      commit / abort       (fabric)
//   X                                          exit
//
// Every request gets exactly one JSON line back:
//   {"rows":[[...],...],"changes":N,"last_insert_rowid":N} | {"txnid":N} | {"ok":true}
//   {"error":"...","code":N}

#define _POSIX_C_SOURCE 200809L
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifdef WEFT_FABRIC
int weft_fdb_start(const char *cluster_file);
void weft_fdb_stop(void);
int weft_vfs_register(int make_default);
int weft_txn_begin(uint64_t *txnid);
int weft_txn_join(sqlite3 *db, uint64_t txnid);
int weft_txn_abort(uint64_t txnid);
int weft_txn_commit(uint64_t txnid);
int weft_txn_recover(void);
#endif

#define MAX_DBS 64
#define LINE_MAX 1 << 20

struct named_db {
	char name[128];
	sqlite3 *db;
};

static struct named_db dbs[MAX_DBS];
static int ndbs = 0;

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int b64_value(char c) {
	const char *p = strchr(b64_table, c);
	return (p && c) ? (int)(p - b64_table) : -1;
}

// Decodes in place-compatible: out must hold 3*len/4 + 1 bytes. Returns the length.
static long b64_decode(const char *in, unsigned char *out) {
	long n = 0;
	int bits = 0, acc = 0;
	for (; *in && *in != '='; in++) {
		int v = b64_value(*in);
		if (v < 0) return -1;
		acc = (acc << 6) | v;
		bits += 6;
		if (bits >= 8) {
			bits -= 8;
			out[n++] = (unsigned char)((acc >> bits) & 0xff);
		}
	}
	out[n] = 0;
	return n;
}

static void json_string(const char *s, long len) {
	putchar('"');
	for (long i = 0; i < len; i++) {
		unsigned char c = (unsigned char)s[i];
		switch (c) {
		case '"': fputs("\\\"", stdout); break;
		case '\\': fputs("\\\\", stdout); break;
		case '\n': fputs("\\n", stdout); break;
		case '\r': fputs("\\r", stdout); break;
		case '\t': fputs("\\t", stdout); break;
		default:
			if (c < 0x20) printf("\\u%04x", c);
			else putchar(c);
		}
	}
	putchar('"');
}

static int reply_error(const char *msg, int code) {
	fputs("{\"error\":", stdout);
	json_string(msg, (long)strlen(msg));
	printf(",\"code\":%d}\n", code);
	fflush(stdout);
	return 0;
}

static sqlite3 *find_db(const char *name) {
	for (int i = 0; i < ndbs; i++)
		if (strcmp(dbs[i].name, name) == 0) return dbs[i].db;
	return NULL;
}

static int apply_pragmas(sqlite3 *db) {
	// The commit is one FoundationDB transaction, so a rollback journal protects nothing,
	// and without EXCLUSIVE every read transaction re-reads page 1 across the network.
	if (sqlite3_exec(db, "PRAGMA journal_mode=MEMORY", NULL, NULL, NULL)) return 1;
	if (sqlite3_exec(db, "PRAGMA locking_mode=EXCLUSIVE", NULL, NULL, NULL)) return 1;
	return 0;
}

static int do_open(char *args) {
	char *name = strtok(args, " ");
	char *enc = strtok(NULL, " ");
	if (!name || !enc) return reply_error("open: expected <name> <base64 path>", 2);
	if (ndbs >= MAX_DBS) return reply_error("open: too many databases", 3);
	if (find_db(name)) {
		puts("{\"ok\":true,\"already_open\":true}");
		fflush(stdout);
		return 0;
	}
	unsigned char *path = malloc(strlen(enc) + 1);
	if (b64_decode(enc, path) < 0) { free(path); return reply_error("open: bad base64", 2); }
	sqlite3 *db = NULL;
#ifdef WEFT_FABRIC
	int rc = sqlite3_open_v2((const char *)path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, "weft_fdb");
#else
	int rc = sqlite3_open_v2((const char *)path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
#endif
	free(path);
	if (rc) {
		char msg[512];
		snprintf(msg, sizeof msg, "open: %s", db ? sqlite3_errmsg(db) : "unknown");
		if (db) sqlite3_close(db);
		return reply_error(msg, rc);
	}
	if (apply_pragmas(db)) {
		char msg[512];
		snprintf(msg, sizeof msg, "pragma: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return reply_error(msg, 1);
	}
	snprintf(dbs[ndbs].name, sizeof dbs[ndbs].name, "%s", name);
	dbs[ndbs].db = db;
	ndbs++;
	puts("{\"ok\":true}");
	fflush(stdout);
	return 0;
}

static int bind_arg(sqlite3_stmt *st, int idx, const char *line) {
	if (strncmp(line, "i:", 2) == 0) return sqlite3_bind_int64(st, idx, strtoll(line + 2, NULL, 10));
	if (strncmp(line, "f:", 2) == 0) return sqlite3_bind_double(st, idx, strtod(line + 2, NULL));
	if (strncmp(line, "n:", 2) == 0) return sqlite3_bind_null(st, idx);
	if (strncmp(line, "s:", 2) == 0 || strncmp(line, "b:", 2) == 0) {
		unsigned char *buf = malloc(strlen(line) + 1);
		long n = b64_decode(line + 2, buf);
		if (n < 0) { free(buf); return SQLITE_MISUSE; }
		int rc = line[0] == 's' ? sqlite3_bind_text(st, idx, (const char *)buf, (int)n, SQLITE_TRANSIENT)
		                        : sqlite3_bind_blob(st, idx, buf, (int)n, SQLITE_TRANSIENT);
		free(buf);
		return rc;
	}
	return SQLITE_MISUSE;
}

static int do_query(char *args, char *line, size_t line_cap) {
	char *name = strtok(args, " ");
	char *enc = strtok(NULL, " ");
	char *argc_s = strtok(NULL, " ");
	if (!name || !enc || !argc_s) return reply_error("query: expected <name> <base64 sql> <argc>", 2);
	int argc = atoi(argc_s);
	sqlite3 *db = find_db(name);
	unsigned char *sql = malloc(strlen(enc) + 1);
	if (b64_decode(enc, sql) < 0) { free(sql); return reply_error("query: bad base64", 2); }
	// Read the argument lines before answering, so the protocol stays in step on error.
	char **argv = calloc((size_t)(argc > 0 ? argc : 1), sizeof(char *));
	for (int i = 0; i < argc; i++) {
		if (!fgets(line, (int)line_cap, stdin)) { free(sql); return reply_error("query: eof in args", 2); }
		line[strcspn(line, "\r\n")] = 0;
		argv[i] = strdup(line);
	}
	if (!db) {
		free(sql);
		for (int i = 0; i < argc; i++) free(argv[i]);
		free(argv);
		return reply_error("query: no such database", 4);
	}
	sqlite3_stmt *st = NULL;
	int rc = sqlite3_prepare_v2(db, (const char *)sql, -1, &st, NULL);
	free(sql);
	if (rc || !st) {
		char msg[512];
		snprintf(msg, sizeof msg, "prepare: %s", sqlite3_errmsg(db));
		for (int i = 0; i < argc; i++) free(argv[i]);
		free(argv);
		return reply_error(msg, rc ? rc : 1);
	}
	for (int i = 0; i < argc; i++) {
		if (bind_arg(st, i + 1, argv[i])) {
			sqlite3_finalize(st);
			for (int j = 0; j < argc; j++) free(argv[j]);
			free(argv);
			return reply_error("bind: bad argument", 2);
		}
	}
	for (int i = 0; i < argc; i++) free(argv[i]);
	free(argv);
	fputs("{\"rows\":[", stdout);
	int cols = sqlite3_column_count(st), first = 1, step;
	while ((step = sqlite3_step(st)) == SQLITE_ROW) {
		if (!first) putchar(',');
		first = 0;
		putchar('[');
		for (int c = 0; c < cols; c++) {
			if (c) putchar(',');
			switch (sqlite3_column_type(st, c)) {
			case SQLITE_INTEGER: printf("%lld", (long long)sqlite3_column_int64(st, c)); break;
			case SQLITE_FLOAT: printf("%.17g", sqlite3_column_double(st, c)); break;
			case SQLITE_NULL: fputs("null", stdout); break;
			default: json_string((const char *)sqlite3_column_text(st, c), sqlite3_column_bytes(st, c));
			}
		}
		putchar(']');
	}
	if (step != SQLITE_DONE) {
		char msg[512];
		snprintf(msg, sizeof msg, "step: %s", sqlite3_errmsg(db));
		sqlite3_finalize(st);
		// The row prefix is already on stdout; close it as an error the reader can spot.
		fputs("],\"error\":", stdout);
		json_string(msg, (long)strlen(msg));
		printf(",\"code\":%d}\n", step);
		fflush(stdout);
		return 0;
	}
	sqlite3_finalize(st);
	printf("],\"changes\":%d,\"last_insert_rowid\":%lld}\n", sqlite3_changes(db),
	       (long long)sqlite3_last_insert_rowid(db));
	fflush(stdout);
	return 0;
}

static int do_txn(char op, char *args) {
#ifdef WEFT_FABRIC
	uint64_t txnid = 0;
	int rc = 0;
	if (op == 'B') {
		rc = weft_txn_begin(&txnid);
		if (rc) return reply_error("txn begin failed", rc);
		printf("{\"txnid\":%llu}\n", (unsigned long long)txnid);
		fflush(stdout);
		return 0;
	}
	if (op == 'J') {
		char *name = strtok(args, " ");
		char *id = strtok(NULL, " ");
		sqlite3 *db = name ? find_db(name) : NULL;
		if (!db || !id) return reply_error("txn join: expected <name> <txnid>", 2);
		rc = weft_txn_join(db, strtoull(id, NULL, 10));
	} else {
		if (!args || !*args) return reply_error("txn: expected <txnid>", 2);
		txnid = strtoull(args, NULL, 10);
		rc = op == 'C' ? weft_txn_commit(txnid) : weft_txn_abort(txnid);
	}
	if (rc) return reply_error("txn failed", rc);
	puts("{\"ok\":true}");
	fflush(stdout);
	return 0;
#else
	(void)op;
	(void)args;
	return reply_error("group commits need the fabric build", 5);
#endif
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;
#ifdef WEFT_FABRIC
	if (weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"))) {
		reply_error("FoundationDB did not start", 1);
		return 1;
	}
	weft_vfs_register(0);
	weft_txn_recover();
#endif
	puts("{\"ready\":true}");
	fflush(stdout);
	size_t cap = LINE_MAX;
	char *line = malloc(cap);
	while (fgets(line, (int)cap, stdin)) {
		line[strcspn(line, "\r\n")] = 0;
		if (!line[0]) continue;
		char op = line[0];
		char *args = line[1] == ' ' ? line + 2 : line + 1;
		switch (op) {
		case 'O': (void)do_open(args); break;
		case 'Q': (void)do_query(args, line, cap); break;
		case 'B': case 'J': case 'C': case 'A': (void)do_txn(op, args); break;
		case 'X': goto done;
		default: (void)reply_error("unknown op", 2);
		}
	}
done:
	for (int i = 0; i < ndbs; i++) sqlite3_close(dbs[i].db);
#ifdef WEFT_FABRIC
	weft_fdb_stop();
#endif
	free(line);
	return 0;
}
