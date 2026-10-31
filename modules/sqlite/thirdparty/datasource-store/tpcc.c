// TPC-C against the store, to find where it stops scaling.
//
//   tpcc load <warehouses>
//   tpcc run  <warehouses> <terminals> <seconds> [--think]
//   tpcc sweep <warehouses> <seconds>
//
// Why TPC-C here, when `bench_vfs` already measures the VFS. `bench_vfs` measures one
// writer doing one kind of work, and its own header says where that ends: "commits in
// flight are worth 44 times, and more database handles are worth nothing". That is a
// statement about a shape of load nobody runs. TPC-C is a shape somebody runs — five
// transaction types in a fixed mix, most of them touching several tables, 1% of new-order
// lines and 15% of payments crossing to another warehouse.
//
// The mapping onto this layout is the reason the benchmark is worth the code:
//
//   one warehouse            one SQLite database, which is the store's unit of ownership
//   a terminal               one thread, committing to its home warehouse
//   a remote line or payment `weft_txn_begin` / `join` / `commit`, the parallel commit
//
// So the remote fraction is not incidental. `prove_parallel_commit` tests the group protocol
// at eight crash points with one grant at a time; this runs it under a mix, concurrently,
// for as long as it is given. A protocol that is correct when a crash lands on it and wrong
// when two of them overlap fails here and passes there.
//
// ITEM is 100,000 rows, read-only, and identical everywhere, so it is one database that
// every thread opens read-only rather than a copy inside each warehouse. That keeps a
// warehouse's database the size the spec says it is, and it keeps ITEM reads off the write
// path entirely.
//
// What is measured. tpmC is new-order transactions that committed, per minute, which is the
// spec's metric and not a synonym for throughput: the other four types are run at their
// required ratios and counted, but only new-order is the number. Latency is reported per
// type at p50, p90 and p99, because a mean over a mix of five transaction types is a number
// about nothing.
//
// The floor is stated in the same table, as `bench_vfs` does: the same run against SQLite on
// a local file. A ratio without a floor is not a measurement.
//
// SPDX-License-Identifier: Apache-2.0
#define _POSIX_C_SOURCE 200809L

#include <sqlite3.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

int weft_fdb_start(const char *cluster_file);
void weft_fdb_stop(void);
int weft_vfs_register(int make_default);
int weft_txn_begin(unsigned long long *txnid);
int weft_txn_join(sqlite3 *db, unsigned long long txnid);
int weft_txn_commit(unsigned long long txnid);
int weft_txn_abort(unsigned long long txnid);

// The spec's cardinalities. They are constants rather than options because a TPC-C result at
// other values is not a TPC-C result, and a flag that silently changes what the number means
// is worse than no flag.
#define DISTRICTS_PER_WAREHOUSE 10
#define CUSTOMERS_PER_DISTRICT  3000
#define ITEMS                   100000
#define ORDERS_PER_DISTRICT     3000  // the initial population; new orders add to it
#define STOCK_PER_WAREHOUSE     ITEMS

// The required mix, 5.2.3. Percentages of the deck, which is shuffled per terminal so the
// order is not the same everywhere.
#define PCT_NEW_ORDER    45
#define PCT_PAYMENT      43
#define PCT_ORDER_STATUS 4
#define PCT_DELIVERY     4
#define PCT_STOCK_LEVEL  4

enum txn_type { NEW_ORDER, PAYMENT, ORDER_STATUS, DELIVERY, STOCK_LEVEL, TXN_TYPES };

static const char *TXN_NAME[TXN_TYPES] = { "new-order", "payment", "order-status",
	                                       "delivery", "stock-level" };

// Keying and think times, 5.2.5.7. Off by default: with them on, a terminal is idle almost
// all of the time and the number measures the sleep rather than the store. They are here
// because a run without them is not a compliant run, and the flag says which was done.
static const double KEYING[TXN_TYPES]     = { 18.0, 3.0, 2.0, 2.0, 2.0 };
static const double THINK_MEAN[TXN_TYPES] = { 12.0, 12.0, 10.0, 5.0, 5.0 };

static double now(void) {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

// --- random, per the spec ----------------------------------------------------------------

// Each thread carries its own state. A shared generator would serialise the terminals on a
// lock and the benchmark would measure the lock.
struct rng {
	unsigned long long s;
};

static unsigned int rnd(struct rng *r) {
	r->s = r->s * 6364136223846793005ULL + 1442695040888963407ULL;
	return (unsigned int)(r->s >> 33);
}

static int uniform(struct rng *r, int lo, int hi) {
	return lo + (int)(rnd(r) % (unsigned int)(hi - lo + 1));
}

// NURand, 2.1.6. The constants C are chosen once per run and must differ between the load
// and the run phases for C_LAST, which is what makes the customer-by-name lookups miss the
// cache the loader warmed. That detail is the reason NURand exists in the spec at all.
static int C_LAST_LOAD = 0, C_LAST_RUN = 0, C_ID_CONST = 0, OL_I_ID_CONST = 0;

static int nurand(struct rng *r, int a, int x, int y, int c) {
	return (((uniform(r, 0, a) | uniform(r, x, y)) + c) % (y - x + 1)) + x;
}

static const char *SYLLABLE[10] = { "BAR", "OUGHT", "ABLE", "PRI",  "PRES",
	                                "ESE", "ANTI",  "CALLY", "ATION", "EING" };

static void last_name(int n, char *out, size_t len) {
	snprintf(out, len, "%s%s%s", SYLLABLE[n / 100], SYLLABLE[(n / 10) % 10], SYLLABLE[n % 10]);
}

static void a_string(struct rng *r, int lo, int hi, char *out, size_t len) {
	int n = uniform(r, lo, hi);
	if ((size_t)n >= len) n = (int)len - 1;
	for (int i = 0; i < n; i++) out[i] = (char)('a' + (int)(rnd(r) % 26));
	out[n] = '\0';
}

// --- sqlite helpers -----------------------------------------------------------------------

static int run(sqlite3 *db, const char *sql) {
	char *err = NULL;
	if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
		fprintf(stderr, "  %s -> %s\n", sql, err ? err : "?");
		sqlite3_free(err);
		return 1;
	}
	return 0;
}

static int runf(sqlite3 *db, const char *fmt, ...) {
	char sql[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(sql, sizeof sql, fmt, ap);
	va_end(ap);
	return run(db, sql);
}

// One scalar out of a query, or `fallback` when the query returned no row. The distinction
// between "no row" and "zero" matters in Delivery, where an empty NEW_ORDER is a legal
// outcome the spec requires be counted rather than treated as an error.
static int scalar(sqlite3 *db, long long *out, const char *fmt, ...) {
	char sql[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(sql, sizeof sql, fmt, ap);
	va_end(ap);

	sqlite3_stmt *st;
	if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK) {
		fprintf(stderr, "prepare: %s -> %s\n", sql, sqlite3_errmsg(db));
		return -1;
	}
	int rc = sqlite3_step(st);
	int found = 0;
	if (rc == SQLITE_ROW) {
		if (sqlite3_column_type(st, 0) != SQLITE_NULL) {
			*out = sqlite3_column_int64(st, 0);
			found = 1;
		}
	} else if (rc != SQLITE_DONE) {
		fprintf(stderr, "step: %s -> %s\n", sql, sqlite3_errmsg(db));
		sqlite3_finalize(st);
		return -1;
	}
	sqlite3_finalize(st);
	return found;
}

static const char *VFS = "weft_fdb";

static sqlite3 *open_db(const char *name, int readonly) {
	sqlite3 *db = NULL;
	int flags = readonly ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
	if (sqlite3_open_v2(name, &db, flags, VFS) != SQLITE_OK) {
		fprintf(stderr, "open %s: %s\n", name, db ? sqlite3_errmsg(db) : "?");
		return NULL;
	}
	run(db, "PRAGMA journal_mode=MEMORY");
	// The same two pragmas `bench_vfs` documents: one owner per database means SQLite can
	// trust its page cache instead of re-reading page 1 for every read transaction, and over
	// a network database that check is a round trip each time.
	run(db, "PRAGMA locking_mode=EXCLUSIVE");
	run(db, "PRAGMA cache_size=-32000");
	return db;
}

static void warehouse_path(char *out, size_t len, int w) {
	snprintf(out, len, "tpcc-w%d.db", w);
}

// --- schema and load ----------------------------------------------------------------------

static const char *SCHEMA =
	"CREATE TABLE IF NOT EXISTS warehouse ("
	"  w_id INTEGER PRIMARY KEY, w_name TEXT, w_street_1 TEXT, w_street_2 TEXT,"
	"  w_city TEXT, w_state TEXT, w_zip TEXT, w_tax REAL, w_ytd REAL);"
	"CREATE TABLE IF NOT EXISTS district ("
	"  d_id INTEGER, d_w_id INTEGER, d_name TEXT, d_street_1 TEXT, d_street_2 TEXT,"
	"  d_city TEXT, d_state TEXT, d_zip TEXT, d_tax REAL, d_ytd REAL,"
	"  d_next_o_id INTEGER, PRIMARY KEY (d_w_id, d_id));"
	"CREATE TABLE IF NOT EXISTS customer ("
	"  c_id INTEGER, c_d_id INTEGER, c_w_id INTEGER, c_first TEXT, c_middle TEXT,"
	"  c_last TEXT, c_street_1 TEXT, c_street_2 TEXT, c_city TEXT, c_state TEXT,"
	"  c_zip TEXT, c_phone TEXT, c_since INTEGER, c_credit TEXT, c_credit_lim REAL,"
	"  c_discount REAL, c_balance REAL, c_ytd_payment REAL, c_payment_cnt INTEGER,"
	"  c_delivery_cnt INTEGER, c_data TEXT, PRIMARY KEY (c_w_id, c_d_id, c_id));"
	// The spec requires customer lookup by last name, taking the middle of the ordered
	// matches. Without this index that is a scan of 3000 rows for 60% of payments and
	// order-status transactions, and the benchmark measures the missing index.
	"CREATE INDEX IF NOT EXISTS customer_last ON customer (c_w_id, c_d_id, c_last, c_first);"
	"CREATE TABLE IF NOT EXISTS history ("
	"  h_c_id INTEGER, h_c_d_id INTEGER, h_c_w_id INTEGER, h_d_id INTEGER,"
	"  h_w_id INTEGER, h_date INTEGER, h_amount REAL, h_data TEXT);"
	"CREATE TABLE IF NOT EXISTS orders ("
	"  o_id INTEGER, o_d_id INTEGER, o_w_id INTEGER, o_c_id INTEGER, o_entry_d INTEGER,"
	"  o_carrier_id INTEGER, o_ol_cnt INTEGER, o_all_local INTEGER,"
	"  PRIMARY KEY (o_w_id, o_d_id, o_id));"
	"CREATE INDEX IF NOT EXISTS orders_cust ON orders (o_w_id, o_d_id, o_c_id, o_id);"
	"CREATE TABLE IF NOT EXISTS new_order ("
	"  no_o_id INTEGER, no_d_id INTEGER, no_w_id INTEGER,"
	"  PRIMARY KEY (no_w_id, no_d_id, no_o_id));"
	"CREATE TABLE IF NOT EXISTS order_line ("
	"  ol_o_id INTEGER, ol_d_id INTEGER, ol_w_id INTEGER, ol_number INTEGER,"
	"  ol_i_id INTEGER, ol_supply_w_id INTEGER, ol_delivery_d INTEGER, ol_quantity INTEGER,"
	"  ol_amount REAL, ol_dist_info TEXT,"
	"  PRIMARY KEY (ol_w_id, ol_d_id, ol_o_id, ol_number));"
	"CREATE TABLE IF NOT EXISTS stock ("
	"  s_i_id INTEGER, s_w_id INTEGER, s_quantity INTEGER, s_dist_01 TEXT, s_dist_02 TEXT,"
	"  s_dist_03 TEXT, s_dist_04 TEXT, s_dist_05 TEXT, s_dist_06 TEXT, s_dist_07 TEXT,"
	"  s_dist_08 TEXT, s_dist_09 TEXT, s_dist_10 TEXT, s_ytd INTEGER, s_order_cnt INTEGER,"
	"  s_remote_cnt INTEGER, s_data TEXT, PRIMARY KEY (s_w_id, s_i_id));";

static const char *ITEM_SCHEMA =
	"CREATE TABLE IF NOT EXISTS item ("
	"  i_id INTEGER PRIMARY KEY, i_im_id INTEGER, i_name TEXT, i_price REAL, i_data TEXT);";

// 10% of item and stock rows carry the string "ORIGINAL" inside i_data / s_data, and the
// new-order transaction's brand-generic flag depends on both. Loading it wrong makes every
// line generic and hides a branch.
static void data_string(struct rng *r, char *out, size_t len, int original) {
	a_string(r, 26, 50, out, len);
	if (original) {
		size_t n = strlen(out);
		if (n > 8) {
			size_t at = (size_t)uniform(r, 0, (int)(n - 8));
			memcpy(out + at, "ORIGINAL", 8);
		}
	}
}

// The loader commits every few thousand rows rather than once per warehouse, and the reason
// is a hard bound in the VFS rather than a preference. `fdb_vfs.c` sets
// MAX_TXN_PIDX = FDB_TXN_LIMIT / PIDX_ROW, which with a 10 MB FoundationDB transaction and a
// 648-byte index row is 15,432 pages, about 63 MB: the index rows of a commit must all land in
// the one transaction that moves the head, so that is the largest commit this store can make
// atomic. A TPC-C warehouse is roughly 60 MB of rows once stock and order lines are counted,
// so a single-transaction load sits on that ceiling and returns SQLITE_IOERR_WRITE, which
// SQLite reports as "disk I/O error" with nothing saying which limit was reached.
//
// Chunking here is honest because the load is not the measured path. The measured path is a
// TPC-C transaction, and the largest of those is a delivery touching ten districts, which is
// nowhere near the bound.
#define LOAD_CHUNK 4000

static int chunk_commit(sqlite3 *db, long long *n) {
	if (++(*n) % LOAD_CHUNK) return 0;
	if (run(db, "COMMIT")) return 1;
	return run(db, "BEGIN");
}

static int load_items(struct rng *r) {
	sqlite3 *db = open_db("tpcc-items.db", 0);
	if (!db) return 1;
	if (run(db, ITEM_SCHEMA)) return 1;
	if (run(db, "DELETE FROM item")) return 1;

	sqlite3_stmt *st;
	if (sqlite3_prepare_v2(db, "INSERT INTO item VALUES (?1,?2,?3,?4,?5)", -1, &st, NULL))
		return 1;
	long long written = 0;
	if (run(db, "BEGIN")) return 1;
	for (int i = 1; i <= ITEMS; i++) {
		char name[32], data[64];
		a_string(r, 14, 24, name, sizeof name);
		data_string(r, data, sizeof data, uniform(r, 1, 10) == 1);
		sqlite3_reset(st);
		sqlite3_bind_int(st, 1, i);
		sqlite3_bind_int(st, 2, uniform(r, 1, 10000));
		sqlite3_bind_text(st, 3, name, -1, SQLITE_TRANSIENT);
		sqlite3_bind_double(st, 4, uniform(r, 100, 10000) / 100.0);
		sqlite3_bind_text(st, 5, data, -1, SQLITE_TRANSIENT);
		if (sqlite3_step(st) != SQLITE_DONE) {
			fprintf(stderr, "item %d: %s\n", i, sqlite3_errmsg(db));
			return 1;
		}
		if (chunk_commit(db, &written)) return 1;
	}
	if (run(db, "COMMIT")) return 1;
	sqlite3_finalize(st);
	sqlite3_close(db);
	return 0;
}

static int load_warehouse(int w, struct rng *r) {
	char path[64];
	warehouse_path(path, sizeof path, w);
	sqlite3 *db = open_db(path, 0);
	if (!db) return 1;
	if (run(db, SCHEMA)) return 1;

	// Chunked, for the reason `chunk_commit` gives: one transaction for a whole warehouse is
	// past the largest commit this store can make atomic.
	long long written = 0;
	if (run(db, "BEGIN")) return 1;

	char s1[32], s2[32], city[32], state[4], zip[16], name[32];
	a_string(r, 6, 10, name, sizeof name);
	a_string(r, 10, 20, s1, sizeof s1);
	a_string(r, 10, 20, s2, sizeof s2);
	a_string(r, 10, 20, city, sizeof city);
	a_string(r, 2, 2, state, sizeof state);
	snprintf(zip, sizeof zip, "%04d11111", uniform(r, 0, 9999));
	if (runf(db, "INSERT OR REPLACE INTO warehouse VALUES (%d,'%s','%s','%s','%s','%s','%s',%f,300000.0)",
	         w, name, s1, s2, city, state, zip, uniform(r, 0, 2000) / 10000.0))
		return 1;

	for (int d = 1; d <= DISTRICTS_PER_WAREHOUSE; d++) {
		a_string(r, 6, 10, name, sizeof name);
		a_string(r, 10, 20, s1, sizeof s1);
		a_string(r, 10, 20, s2, sizeof s2);
		a_string(r, 10, 20, city, sizeof city);
		a_string(r, 2, 2, state, sizeof state);
		snprintf(zip, sizeof zip, "%04d11111", uniform(r, 0, 9999));
		if (runf(db,
		         "INSERT OR REPLACE INTO district VALUES (%d,%d,'%s','%s','%s','%s','%s','%s',%f,30000.0,%d)",
		         d, w, name, s1, s2, city, state, zip, uniform(r, 0, 2000) / 10000.0,
		         ORDERS_PER_DISTRICT + 1))
			return 1;

		for (int c = 1; c <= CUSTOMERS_PER_DISTRICT; c++) {
			char first[32], last[32], phone[24], cdata[600];
			a_string(r, 8, 16, first, sizeof first);
			// The first 1000 customers get every last name once, the rest are NURand.
			// That is what makes a name lookup return more than one row, which is the
			// case the "middle of the ordered matches" rule exists for.
			last_name(c <= 1000 ? c - 1 : nurand(r, 255, 0, 999, C_LAST_LOAD), last,
			          sizeof last);
			a_string(r, 10, 20, s1, sizeof s1);
			a_string(r, 10, 20, s2, sizeof s2);
			a_string(r, 10, 20, city, sizeof city);
			a_string(r, 2, 2, state, sizeof state);
			snprintf(zip, sizeof zip, "%04d11111", uniform(r, 0, 9999));
			for (int i = 0; i < 16; i++) phone[i] = (char)('0' + (int)(rnd(r) % 10));
			phone[16] = '\0';
			a_string(r, 300, 500, cdata, sizeof cdata);
			if (runf(db,
			         "INSERT OR REPLACE INTO customer VALUES "
			         "(%d,%d,%d,'%s','OE','%s','%s','%s','%s','%s','%s','%s',0,'%s',"
			         "50000.0,%f,-10.0,10.0,1,0,'%s')",
			         c, d, w, first, last, s1, s2, city, state, zip, phone,
			         uniform(r, 1, 10) == 1 ? "BC" : "GC",
			         uniform(r, 0, 5000) / 10000.0, cdata))
				return 1;
			if (chunk_commit(db, &written)) return 1;
		}

		// Orders are loaded in a permuted customer order, so o_c_id is not c_id. The
		// permutation is what stops order-status from being a primary-key lookup in
		// disguise.
		int *perm = malloc(sizeof(int) * (size_t)ORDERS_PER_DISTRICT);
		if (!perm) return 1;
		for (int i = 0; i < ORDERS_PER_DISTRICT; i++) perm[i] = i + 1;
		for (int i = ORDERS_PER_DISTRICT - 1; i > 0; i--) {
			int j = uniform(r, 0, i);
			int t = perm[i];
			perm[i] = perm[j];
			perm[j] = t;
		}

		for (int o = 1; o <= ORDERS_PER_DISTRICT; o++) {
			int ol_cnt = uniform(r, 5, 15);
			// The last 900 orders of each district are undelivered: they have no carrier
			// and they are the rows NEW_ORDER holds. Delivery consumes them.
			int delivered = o <= 2100;
			if (runf(db, "INSERT OR REPLACE INTO orders VALUES (%d,%d,%d,%d,0,%s,%d,1)", o,
			         d, w, perm[o - 1], delivered ? "1" : "NULL", ol_cnt))
				return 1;
			if (!delivered &&
			    runf(db, "INSERT OR REPLACE INTO new_order VALUES (%d,%d,%d)", o, d, w))
				return 1;
			for (int n = 1; n <= ol_cnt; n++) {
				char dist[32];
				a_string(r, 24, 24, dist, sizeof dist);
				if (runf(db,
				         "INSERT OR REPLACE INTO order_line VALUES "
				         "(%d,%d,%d,%d,%d,%d,%s,5,%f,'%s')",
				         o, d, w, n, uniform(r, 1, ITEMS), w, delivered ? "0" : "NULL",
				         delivered ? 0.0 : uniform(r, 1, 999999) / 100.0, dist))
					return 1;
				if (chunk_commit(db, &written)) return 1;
			}
		}
		free(perm);
	}

	for (int i = 1; i <= STOCK_PER_WAREHOUSE; i++) {
		char d[10][32], sdata[64];
		for (int k = 0; k < 10; k++) a_string(r, 24, 24, d[k], sizeof d[k]);
		data_string(r, sdata, sizeof sdata, uniform(r, 1, 10) == 1);
		if (runf(db,
		         "INSERT OR REPLACE INTO stock VALUES "
		         "(%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',0,0,0,'%s')",
		         i, w, uniform(r, 10, 100), d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
		         d[8], d[9], sdata))
			return 1;
		if (chunk_commit(db, &written)) return 1;
	}

	if (run(db, "COMMIT")) return 1;
	sqlite3_close(db);
	return 0;
}

// --- the five transactions ------------------------------------------------------------------

// A terminal. It owns its home warehouse's handle and opens a remote warehouse's handle only
// when it needs one, caching them, because the store's rule is one owner for one database and
// a handle is cheap next to a commit.
struct terminal {
	int id;
	int home_w;
	int warehouses;
	struct rng rng;
	sqlite3 *db;    // home warehouse
	sqlite3 *items; // read-only, shared schema, one handle each
	sqlite3 **remote;
	int think;

	long long count[TXN_TYPES];
	long long failed[TXN_TYPES];
	long long remote_groups; // parallel commits actually performed
	double *lat[TXN_TYPES];
	long long lat_n[TXN_TYPES], lat_cap[TXN_TYPES];
};

// --- one owner for each database ------------------------------------------------------
//
// The rule this implements is the store's own: a database has one owner. `open_db` sets
// `PRAGMA locking_mode=EXCLUSIVE` on that basis, so SQLite may trust its page cache instead
// of re-reading page 1 for every read transaction -- which over a network database is a
// round trip each time.
//
// The benchmark used to break the rule two different ways, and either one was enough. It
// opened a handle for each terminal, so terminals sharing a warehouse had two owners; and
// `remote_handle` opened a second writer to a warehouse another terminal already owned. The
// symptom at two terminals was `COMMIT -> attempt to write a readonly database`, and at four
// it was `malloc(): unaligned tcache chunk detected` -- a heap corruption, which is the worse
// outcome because it has no error to catch.
//
// So: one handle for each database, and a lock in front of it. Terminals contend on the lock
// rather than duplicating the handle. What this measures changes with it, and the change is
// worth stating rather than discovering later -- concurrency is now bounded by the number of
// *warehouses*, not the number of terminals. Ten terminals against one warehouse is a queue,
// as it should be, because they are all writing the same database.
//
// Index 0 is the items database, which is read-only and shared. Indexes 1..W are warehouses.
// Locks are always taken in ascending index order, so no two threads can hold one lock each
// and wait for the other's. That total order is what makes this deadlock-free, and it is why
// a transaction must know every database it touches *before* it takes the first lock.
#define WH_ITEMS 0
static sqlite3 **WH_DB;
static pthread_mutex_t *WH_LOCK;
static int WH_N; // warehouses; the array holds WH_N + 1 entries

// What this thread holds, so it can be released at one place rather than at each of the
// several `goto` exits every transaction has.
static __thread int held[24];
static __thread int held_n;

static int cmp_int(const void *a, const void *b) {
	int x = *(const int *)a, y = *(const int *)b;
	return (x > y) - (x < y);
}

// Lock the given set of databases, ascending, each one once.
static void wh_lock(const int *want, int n) {
	int ws[24];
	if (n > (int)(sizeof ws / sizeof *ws)) n = (int)(sizeof ws / sizeof *ws);
	memcpy(ws, want, sizeof(int) * (size_t)n);
	qsort(ws, (size_t)n, sizeof(int), cmp_int);
	held_n = 0;
	for (int i = 0; i < n; i++) {
		if (i && ws[i] == ws[i - 1]) continue; // the same warehouse on two order lines
		if (ws[i] < 0 || ws[i] > WH_N) continue;
		pthread_mutex_lock(&WH_LOCK[ws[i]]);
		held[held_n++] = ws[i];
	}
}

static void wh_unlock_all(void) {
	while (held_n > 0) pthread_mutex_unlock(&WH_LOCK[held[--held_n]]);
}

static sqlite3 *remote_handle(struct terminal *t, int w) {
	(void)t;
	// One handle for each warehouse, opened once in `run_once`. This used to open a second
	// writer here, which is the defect described above.
	if (w < 1 || w > WH_N) return NULL;
	return WH_DB[w];
}

static void record(struct terminal *t, enum txn_type type, double seconds, int ok) {
	if (!ok) {
		t->failed[type]++;
		return;
	}
	t->count[type]++;
	if (t->lat_n[type] == t->lat_cap[type]) {
		long long cap = t->lat_cap[type] ? t->lat_cap[type] * 2 : 1024;
		double *grown = realloc(t->lat[type], sizeof(double) * (size_t)cap);
		if (!grown) return; // a lost sample is better than a dead terminal
		t->lat[type] = grown;
		t->lat_cap[type] = cap;
	}
	t->lat[type][t->lat_n[type]++] = seconds;
}

// 2.4. Five to fifteen lines, 1% of them supplied by a remote warehouse, and 1% of the
// transactions rolled back on purpose by asking for an item that does not exist. Both of
// those are required, and both are the interesting paths: the remote line is a parallel
// commit, and the rollback is the only place the harness proves a transaction can be undone.
static int tx_new_order(struct terminal *t) {
	int w = t->home_w;
	int d = uniform(&t->rng, 1, DISTRICTS_PER_WAREHOUSE);
	int c = nurand(&t->rng, 1023, 1, CUSTOMERS_PER_DISTRICT, C_ID_CONST);
	int ol_cnt = uniform(&t->rng, 5, 15);
	int rollback = uniform(&t->rng, 1, 100) == 1;

	int item[15], supply_w[15], qty[15];
	int all_local = 1;
	for (int i = 0; i < ol_cnt; i++) {
		item[i] = nurand(&t->rng, 8191, 1, ITEMS, OL_I_ID_CONST);
		if (rollback && i == ol_cnt - 1) item[i] = ITEMS + 1; // the unused item id
		supply_w[i] = w;
		if (t->warehouses > 1 && uniform(&t->rng, 1, 100) == 1) {
			do {
				supply_w[i] = uniform(&t->rng, 1, t->warehouses);
			} while (supply_w[i] == w);
			all_local = 0;
		}
		qty[i] = uniform(&t->rng, 1, 10);
	}

	// Every database this transaction touches, locked before the first statement. The set
	// has to be complete here: taking a lock later, after work has begun, is how a deadlock
	// is built.
	{
		int set[17];
		int n = 0;
		set[n++] = WH_ITEMS;
		set[n++] = w;
		for (int i = 0; i < ol_cnt && n < 17; i++) set[n++] = supply_w[i];
		wh_lock(set, n);
	}

	// Which remote warehouses take part decides whether this is one commit or a group.
	unsigned long long txnid = 0;
	int grouped = 0;
	if (!all_local) {
		if (weft_txn_begin(&txnid) != SQLITE_OK) return 0;
		grouped = 1;
		if (weft_txn_join(t->db, txnid) != SQLITE_OK) goto give_up;
		for (int i = 0; i < ol_cnt; i++) {
			if (supply_w[i] == w) continue;
			sqlite3 *r = remote_handle(t, supply_w[i]);
			if (!r) goto give_up;
			// Joining twice is not an error the protocol has to tolerate, so the same
			// warehouse appearing on two lines joins once.
			int already = 0;
			for (int k = 0; k < i; k++)
				if (supply_w[k] == supply_w[i]) already = 1;
			if (!already && weft_txn_join(r, txnid) != SQLITE_OK) goto give_up;
		}
	}

	if (run(t->db, "BEGIN")) goto give_up;

	long long d_next = 0, ok;
	ok = scalar(t->db, &d_next, "SELECT d_next_o_id FROM district WHERE d_w_id=%d AND d_id=%d",
	            w, d);
	if (ok != 1) goto rollback_home;
	if (runf(t->db, "UPDATE district SET d_next_o_id=%lld WHERE d_w_id=%d AND d_id=%d",
	         d_next + 1, w, d))
		goto rollback_home;
	if (runf(t->db, "INSERT INTO orders VALUES (%lld,%d,%d,%d,%lld,NULL,%d,%d)", d_next, d, w, c,
	         (long long)time(NULL), ol_cnt, all_local))
		goto rollback_home;
	if (runf(t->db, "INSERT INTO new_order VALUES (%lld,%d,%d)", d_next, d, w))
		goto rollback_home;

	for (int i = 0; i < ol_cnt; i++) {
		long long price_cents = 0;
		int found = scalar(t->items, &price_cents,
		                   "SELECT CAST(i_price*100 AS INTEGER) FROM item WHERE i_id=%d",
		                   item[i]);
		if (found != 1) {
			// The deliberate rollback. Everything written so far must go, which is the
			// only assertion in this transaction that is about the store rather than
			// about TPC-C.
			goto rollback_home;
		}
		sqlite3 *sdb = remote_handle(t, supply_w[i]);
		if (!sdb) goto rollback_home;
		if (supply_w[i] != w && run(sdb, "BEGIN")) goto rollback_home;
		if (runf(sdb,
		         "UPDATE stock SET s_quantity = CASE WHEN s_quantity >= %d+10"
		         " THEN s_quantity-%d ELSE s_quantity-%d+91 END,"
		         " s_ytd = s_ytd+%d, s_order_cnt = s_order_cnt+1, s_remote_cnt = s_remote_cnt+%d"
		         " WHERE s_w_id=%d AND s_i_id=%d",
		         qty[i], qty[i], qty[i], qty[i], supply_w[i] == w ? 0 : 1, supply_w[i],
		         item[i]))
			goto rollback_home;
		if (runf(t->db,
		         "INSERT INTO order_line VALUES (%lld,%d,%d,%d,%d,%d,NULL,%d,%f,'')", d_next,
		         d, w, i + 1, item[i], supply_w[i], qty[i],
		         (double)price_cents * qty[i] / 100.0))
			goto rollback_home;
	}

	if (run(t->db, "COMMIT")) goto give_up;
	if (grouped) {
		for (int i = 0; i < ol_cnt; i++) {
			if (supply_w[i] == w) continue;
			int already = 0;
			for (int k = 0; k < i; k++)
				if (supply_w[k] == supply_w[i]) already = 1;
			if (!already && run(remote_handle(t, supply_w[i]), "COMMIT")) goto give_up;
		}
		if (weft_txn_commit(txnid) != SQLITE_OK) return 0;
		t->remote_groups++;
	}
	return 1;

rollback_home:
	run(t->db, "ROLLBACK");
	for (int i = 0; i < ol_cnt; i++)
		if (supply_w[i] != w && t->remote[supply_w[i]]) run(t->remote[supply_w[i]], "ROLLBACK");
	if (grouped) weft_txn_abort(txnid);
	// A rolled-back new-order is a *completed* transaction under the spec, and it counts
	// toward tpmC. Reporting it as a failure would understate the rate by 1% and, worse,
	// would make a broken rollback path look like a slow one.
	return 1;

give_up:
	run(t->db, "ROLLBACK");
	if (grouped) weft_txn_abort(txnid);
	return 0;
}

// 2.5. 15% of payments are to a customer of a remote warehouse, which is the second place
// the group protocol runs. 60% select the customer by last name.
static int tx_payment(struct terminal *t) {
	int w = t->home_w;
	int d = uniform(&t->rng, 1, DISTRICTS_PER_WAREHOUSE);
	int c_w = w, c_d = d;
	if (t->warehouses > 1 && uniform(&t->rng, 1, 100) <= 15) {
		do {
			c_w = uniform(&t->rng, 1, t->warehouses);
		} while (c_w == w);
		c_d = uniform(&t->rng, 1, DISTRICTS_PER_WAREHOUSE);
	}
	double amount = uniform(&t->rng, 100, 500000) / 100.0;
	int by_name = uniform(&t->rng, 1, 100) <= 60;

	{
		int set[2] = { w, c_w };
		wh_lock(set, 2);
	}

	sqlite3 *cdb = remote_handle(t, c_w);
	if (!cdb) return 0;

	unsigned long long txnid = 0;
	int grouped = 0;
	if (c_w != w) {
		if (weft_txn_begin(&txnid) != SQLITE_OK) return 0;
		grouped = 1;
		if (weft_txn_join(t->db, txnid) != SQLITE_OK) goto give_up;
		if (weft_txn_join(cdb, txnid) != SQLITE_OK) goto give_up;
	}

	if (run(t->db, "BEGIN")) goto give_up;
	if (c_w != w && run(cdb, "BEGIN")) goto give_up;

	if (runf(t->db, "UPDATE warehouse SET w_ytd = w_ytd + %f WHERE w_id=%d", amount, w))
		goto undo;
	if (runf(t->db, "UPDATE district SET d_ytd = d_ytd + %f WHERE d_w_id=%d AND d_id=%d",
	         amount, w, d))
		goto undo;

	long long c_id = 0;
	if (by_name) {
		char last[32];
		last_name(nurand(&t->rng, 255, 0, 999, C_LAST_RUN), last, sizeof last);
		long long n = 0;
		if (scalar(cdb, &n, "SELECT count(*) FROM customer WHERE c_w_id=%d AND c_d_id=%d"
		                    " AND c_last='%s'",
		           c_w, c_d, last) != 1)
			goto undo;
		if (n == 0) {
			// No customer with that name in this district. The spec's population makes
			// this possible, and it is not an error: the transaction picks by id instead
			// rather than reporting a failure that would be counted as one.
			c_id = nurand(&t->rng, 1023, 1, CUSTOMERS_PER_DISTRICT, C_ID_CONST);
		} else {
			// "the middle one of the ordered set", 2.5.2.2.
			if (scalar(cdb, &c_id,
			           "SELECT c_id FROM customer WHERE c_w_id=%d AND c_d_id=%d AND"
			           " c_last='%s' ORDER BY c_first LIMIT 1 OFFSET %lld",
			           c_w, c_d, last, n / 2) != 1)
				goto undo;
		}
	} else {
		c_id = nurand(&t->rng, 1023, 1, CUSTOMERS_PER_DISTRICT, C_ID_CONST);
	}

	if (runf(cdb,
	         "UPDATE customer SET c_balance = c_balance - %f, c_ytd_payment = c_ytd_payment + %f,"
	         " c_payment_cnt = c_payment_cnt + 1 WHERE c_w_id=%d AND c_d_id=%d AND c_id=%lld",
	         amount, amount, c_w, c_d, c_id))
		goto undo;

	// Bad-credit customers carry the last four payments in c_data, 2.5.2.2. Skipping this
	// makes 10% of payments cheaper than they are.
	long long bad = 0;
	if (scalar(cdb, &bad,
	           "SELECT c_credit='BC' FROM customer WHERE c_w_id=%d AND c_d_id=%d AND c_id=%lld",
	           c_w, c_d, c_id) == 1 &&
	    bad) {
		if (runf(cdb,
		         "UPDATE customer SET c_data = substr(printf('%%d %%d %%d %%d %%f ',%lld,%d,%d,"
		         "%d,%f) || c_data, 1, 500) WHERE c_w_id=%d AND c_d_id=%d AND c_id=%lld",
		         c_id, c_d, c_w, d, amount, c_w, c_d, c_id))
			goto undo;
	}

	if (runf(t->db, "INSERT INTO history VALUES (%lld,%d,%d,%d,%d,%lld,%f,'')", c_id, c_d, c_w,
	         d, w, (long long)time(NULL), amount))
		goto undo;

	if (run(t->db, "COMMIT")) goto give_up;
	if (c_w != w) {
		if (run(cdb, "COMMIT")) goto give_up;
		if (weft_txn_commit(txnid) != SQLITE_OK) return 0;
		t->remote_groups++;
	}
	return 1;

undo:
	run(t->db, "ROLLBACK");
	if (c_w != w) run(cdb, "ROLLBACK");
	if (grouped) weft_txn_abort(txnid);
	return 0;

give_up:
	if (grouped) weft_txn_abort(txnid);
	return 0;
}

// 2.6. Read-only: the customer's last order and its lines. It is here because a mix of only
// writes measures the commit path and nothing else, and because it is the transaction whose
// cost the read-ahead window in `spec/ReadAhead.lean` is about.
static int tx_order_status(struct terminal *t) {
	{ int set[1] = { t->home_w }; wh_lock(set, 1); }
	int w = t->home_w;
	int d = uniform(&t->rng, 1, DISTRICTS_PER_WAREHOUSE);
	int by_name = uniform(&t->rng, 1, 100) <= 60;
	long long c_id = 0;

	if (by_name) {
		char last[32];
		last_name(nurand(&t->rng, 255, 0, 999, C_LAST_RUN), last, sizeof last);
		long long n = 0;
		if (scalar(t->db, &n,
		           "SELECT count(*) FROM customer WHERE c_w_id=%d AND c_d_id=%d AND c_last='%s'",
		           w, d, last) != 1)
			return 0;
		if (n == 0)
			c_id = nurand(&t->rng, 1023, 1, CUSTOMERS_PER_DISTRICT, C_ID_CONST);
		else if (scalar(t->db, &c_id,
		                "SELECT c_id FROM customer WHERE c_w_id=%d AND c_d_id=%d AND"
		                " c_last='%s' ORDER BY c_first LIMIT 1 OFFSET %lld",
		                w, d, last, n / 2) != 1)
			return 0;
	} else {
		c_id = nurand(&t->rng, 1023, 1, CUSTOMERS_PER_DISTRICT, C_ID_CONST);
	}

	long long o_id = 0;
	int found = scalar(t->db, &o_id,
	                   "SELECT o_id FROM orders WHERE o_w_id=%d AND o_d_id=%d AND o_c_id=%lld"
	                   " ORDER BY o_id DESC LIMIT 1",
	                   w, d, c_id);
	if (found < 0) return 0;
	if (found == 0) return 1; // a customer with no orders is a legal outcome

	long long lines = 0;
	if (scalar(t->db, &lines,
	           "SELECT count(*) FROM order_line WHERE ol_w_id=%d AND ol_d_id=%d AND ol_o_id=%lld",
	           w, d, o_id) != 1)
		return 0;
	return 1;
}

// 2.7. The oldest undelivered order in each of the ten districts, delivered in one go. This
// is the heaviest write transaction in the mix and the one that moves NEW_ORDER's floor.
static int tx_delivery(struct terminal *t) {
	{ int set[1] = { t->home_w }; wh_lock(set, 1); }
	int w = t->home_w;
	int carrier = uniform(&t->rng, 1, 10);
	int delivered = 0;

	if (run(t->db, "BEGIN")) return 0;
	for (int d = 1; d <= DISTRICTS_PER_WAREHOUSE; d++) {
		long long o_id = 0;
		int found = scalar(t->db, &o_id,
		                   "SELECT no_o_id FROM new_order WHERE no_w_id=%d AND no_d_id=%d"
		                   " ORDER BY no_o_id LIMIT 1",
		                   w, d);
		if (found < 0) goto undo;
		// "If no matching row is found, the delivery of this district is skipped", 2.7.4.2.
		// Counting that as a failure would turn a drained district into a broken store.
		if (found == 0) continue;

		if (runf(t->db, "DELETE FROM new_order WHERE no_w_id=%d AND no_d_id=%d AND no_o_id=%lld",
		         w, d, o_id))
			goto undo;
		long long c_id = 0;
		if (scalar(t->db, &c_id,
		           "SELECT o_c_id FROM orders WHERE o_w_id=%d AND o_d_id=%d AND o_id=%lld", w,
		           d, o_id) != 1)
			goto undo;
		if (runf(t->db,
		         "UPDATE orders SET o_carrier_id=%d WHERE o_w_id=%d AND o_d_id=%d AND o_id=%lld",
		         carrier, w, d, o_id))
			goto undo;
		if (runf(t->db,
		         "UPDATE order_line SET ol_delivery_d=%lld WHERE ol_w_id=%d AND ol_d_id=%d"
		         " AND ol_o_id=%lld",
		         (long long)time(NULL), w, d, o_id))
			goto undo;
		long long amount = 0;
		if (scalar(t->db, &amount,
		           "SELECT CAST(sum(ol_amount)*100 AS INTEGER) FROM order_line WHERE ol_w_id=%d"
		           " AND ol_d_id=%d AND ol_o_id=%lld",
		           w, d, o_id) != 1)
			goto undo;
		if (runf(t->db,
		         "UPDATE customer SET c_balance = c_balance + %f, c_delivery_cnt ="
		         " c_delivery_cnt + 1 WHERE c_w_id=%d AND c_d_id=%d AND c_id=%lld",
		         (double)amount / 100.0, w, d, c_id))
			goto undo;
		delivered++;
	}
	if (run(t->db, "COMMIT")) return 0;
	return 1;

undo:
	run(t->db, "ROLLBACK");
	return 0;
}

// 2.8. How many of the items in the last 20 orders of a district are below a stock threshold.
// Read-only and the only transaction that scans, which is why it is the one that shows what
// the read-ahead window is worth.
static int tx_stock_level(struct terminal *t) {
	{ int set[1] = { t->home_w }; wh_lock(set, 1); }
	int w = t->home_w;
	int d = uniform(&t->rng, 1, DISTRICTS_PER_WAREHOUSE);
	int threshold = uniform(&t->rng, 10, 20);

	long long next = 0;
	if (scalar(t->db, &next, "SELECT d_next_o_id FROM district WHERE d_w_id=%d AND d_id=%d", w,
	           d) != 1)
		return 0;

	long long low = 0;
	if (scalar(t->db, &low,
	           "SELECT count(DISTINCT ol_i_id) FROM order_line JOIN stock ON s_i_id = ol_i_id"
	           " AND s_w_id = %d WHERE ol_w_id=%d AND ol_d_id=%d AND ol_o_id >= %lld"
	           " AND ol_o_id < %lld AND s_quantity < %d",
	           w, w, d, next - 20, next, threshold) != 1)
		return 0;
	return 1;
}

// --- the terminal loop ----------------------------------------------------------------------

static volatile int stop = 0;

struct plan {
	int warehouses;
	int terminals;
	double seconds;
	int think;
};

static void sleep_seconds(double s) {
	if (s <= 0) return;
	struct timespec t = { (time_t)s, (long)((s - (double)(time_t)s) * 1e9) };
	nanosleep(&t, NULL);
}

// The deck: 100 entries in the required proportions, shuffled. Drawing a type at random with
// the right probabilities would be simpler and would let a short run drift far from the mix;
// a shuffled deck is exact over each hundred transactions, which is what 5.2.3 asks for.
static void deal(struct rng *r, enum txn_type *deck) {
	int at = 0;
	for (int i = 0; i < PCT_NEW_ORDER; i++) deck[at++] = NEW_ORDER;
	for (int i = 0; i < PCT_PAYMENT; i++) deck[at++] = PAYMENT;
	for (int i = 0; i < PCT_ORDER_STATUS; i++) deck[at++] = ORDER_STATUS;
	for (int i = 0; i < PCT_DELIVERY; i++) deck[at++] = DELIVERY;
	for (int i = 0; i < PCT_STOCK_LEVEL; i++) deck[at++] = STOCK_LEVEL;
	for (int i = 99; i > 0; i--) {
		int j = uniform(r, 0, i);
		enum txn_type t = deck[i];
		deck[i] = deck[j];
		deck[j] = t;
	}
}

static void *terminal_main(void *arg) {
	struct terminal *t = arg;
	enum txn_type deck[100];
	int at = 100;

	while (!stop) {
		if (at == 100) {
			deal(&t->rng, deck);
			at = 0;
		}
		enum txn_type type = deck[at++];

		if (t->think) sleep_seconds(KEYING[type]);

		double t0 = now();
		int ok;
		switch (type) {
			case NEW_ORDER: ok = tx_new_order(t); break;
			case PAYMENT: ok = tx_payment(t); break;
			case ORDER_STATUS: ok = tx_order_status(t); break;
			case DELIVERY: ok = tx_delivery(t); break;
			default: ok = tx_stock_level(t); break;
		}
		// One place to release, and it must be here rather than inside the transactions:
		// each of them has several `goto` exits, and an unlock missing from one of those
		// paths is a hang that appears only under load.
		wh_unlock_all();

		record(t, type, now() - t0, ok);

		if (t->think) {
			// Negative exponential, mean per type, clamped at 10x the mean as 5.2.5.4
			// requires. Without the clamp one draw in a short run can be minutes long.
			double u = (double)(rnd(&t->rng) % 1000000u + 1) / 1000001.0;
			double think = -THINK_MEAN[type] * log(u);
			if (think > 10 * THINK_MEAN[type]) think = 10 * THINK_MEAN[type];
			sleep_seconds(think);
		}
	}
	return NULL;
}

// --- reporting --------------------------------------------------------------------------

static int cmp_double(const void *a, const void *b) {
	double x = *(const double *)a, y = *(const double *)b;
	return x < y ? -1 : x > y ? 1 : 0;
}

// The percentile of a sorted array, nearest-rank. Reported rather than a mean, because the
// mix has five transaction types whose costs differ by more than an order of magnitude and a
// mean over them is a number about nothing.
static double pct(double *v, long long n, double p) {
	if (n == 0) return 0.0;
	long long at = (long long)(p * (double)n);
	if (at >= n) at = n - 1;
	return v[at];
}

struct totals {
	long long count[TXN_TYPES], failed[TXN_TYPES], remote_groups;
	double *lat[TXN_TYPES];
	long long lat_n[TXN_TYPES];
};

static void report(struct totals *tot, double elapsed, int warehouses, int terminals,
                   const char *what) {
	printf("\n%s: %d warehouses, %d terminals, %.0fs\n", what, warehouses, terminals, elapsed);
	printf("%-14s %10s %8s %8s %9s %9s %9s\n", "transaction", "count", "share", "failed",
	       "p50 ms", "p90 ms", "p99 ms");

	long long total = 0;
	for (int i = 0; i < TXN_TYPES; i++) total += tot->count[i];

	for (int i = 0; i < TXN_TYPES; i++) {
		qsort(tot->lat[i], (size_t)tot->lat_n[i], sizeof(double), cmp_double);
		printf("%-14s %10lld %7.1f%% %8lld %9.2f %9.2f %9.2f\n", TXN_NAME[i], tot->count[i],
		       total ? 100.0 * (double)tot->count[i] / (double)total : 0.0, tot->failed[i],
		       pct(tot->lat[i], tot->lat_n[i], 0.50) * 1000.0,
		       pct(tot->lat[i], tot->lat_n[i], 0.90) * 1000.0,
		       pct(tot->lat[i], tot->lat_n[i], 0.99) * 1000.0);
	}

	// tpmC is new-order per minute and nothing else, 5.6.1. The share column above is what
	// says whether the run was a TPC-C run at all: if new-order is not near 45%, the number
	// below is not tpmC whatever it is called.
	double tpmc = (double)tot->count[NEW_ORDER] / elapsed * 60.0;
	printf("\ntpmC %.1f   (new-order share %.1f%%, required 45%%)\n", tpmc,
	       total ? 100.0 * (double)tot->count[NEW_ORDER] / (double)total : 0.0);
	printf("parallel commits: %lld   (%.2f%% of new-order and payment)\n", tot->remote_groups,
	       (tot->count[NEW_ORDER] + tot->count[PAYMENT])
	           ? 100.0 * (double)tot->remote_groups
	                 / (double)(tot->count[NEW_ORDER] + tot->count[PAYMENT])
	           : 0.0);
}

// --- run --------------------------------------------------------------------------------

static double run_once(int warehouses, int terminals, double seconds, int think,
                       struct totals *tot) {
	struct terminal *ts = calloc((size_t)terminals, sizeof *ts);
	pthread_t *th = calloc((size_t)terminals, sizeof *th);
	if (!ts || !th) return -1;

	// One handle and one lock for each database, before any thread exists.
	WH_N = warehouses;
	WH_DB = calloc((size_t)warehouses + 1, sizeof *WH_DB);
	WH_LOCK = calloc((size_t)warehouses + 1, sizeof *WH_LOCK);
	if (!WH_DB || !WH_LOCK) return -1;
	for (int w = 0; w <= warehouses; w++) pthread_mutex_init(&WH_LOCK[w], NULL);
	WH_DB[WH_ITEMS] = open_db("tpcc-items.db", 1);
	for (int w = 1; w <= warehouses; w++) {
		char path[64];
		warehouse_path(path, sizeof path, w);
		WH_DB[w] = open_db(path, 0);
		if (!WH_DB[w]) {
			fprintf(stderr, "could not open warehouse %d\n", w);
			return -1;
		}
	}

	for (int i = 0; i < terminals; i++) {
		ts[i].id = i;
		// Terminals are spread over the warehouses round-robin. With terminals > warehouses
		// several terminals share a database, which is the case that finds out whether the
		// one-owner rule costs anything under contention.
		ts[i].home_w = (i % warehouses) + 1;
		ts[i].warehouses = warehouses;
		ts[i].rng.s = 0x9e3779b97f4a7c15ULL * (unsigned long long)(i + 1);
		ts[i].think = think;
		ts[i].remote = WH_DB; // shared; kept as a field so the rest of the file is unchanged
		ts[i].db = WH_DB[ts[i].home_w];
		ts[i].items = WH_DB[WH_ITEMS];
		if (!ts[i].db || !ts[i].items) {
			fprintf(stderr, "terminal %d could not open its databases\n", i);
			return -1;
		}
	}

	stop = 0;
	double t0 = now();
	for (int i = 0; i < terminals; i++) pthread_create(&th[i], NULL, terminal_main, &ts[i]);
	sleep_seconds(seconds);
	stop = 1;
	for (int i = 0; i < terminals; i++) pthread_join(th[i], NULL);
	double elapsed = now() - t0;

	for (int i = 0; i < terminals; i++) {
		tot->remote_groups += ts[i].remote_groups;
		for (int k = 0; k < TXN_TYPES; k++) {
			tot->count[k] += ts[i].count[k];
			tot->failed[k] += ts[i].failed[k];
			long long n = tot->lat_n[k] + ts[i].lat_n[k];
			double *grown = realloc(tot->lat[k], sizeof(double) * (size_t)(n ? n : 1));
			if (grown) {
				tot->lat[k] = grown;
				memcpy(tot->lat[k] + tot->lat_n[k], ts[i].lat[k],
				       sizeof(double) * (size_t)ts[i].lat_n[k]);
				tot->lat_n[k] = n;
			}
			free(ts[i].lat[k]);
		}
	}

	// The handles are shared now, so they are closed once here rather than once for each
	// terminal. Closing them in the loop above would have closed each database as many times
	// as there were terminals on it.
	for (int w = 0; w <= warehouses; w++) {
		if (WH_DB[w]) sqlite3_close(WH_DB[w]);
		pthread_mutex_destroy(&WH_LOCK[w]);
	}
	free(WH_DB);
	free(WH_LOCK);
	WH_DB = NULL;
	WH_LOCK = NULL;
	free(ts);
	free(th);
	return elapsed;
}

static void free_totals(struct totals *t) {
	for (int i = 0; i < TXN_TYPES; i++) free(t->lat[i]);
	memset(t, 0, sizeof *t);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr,
		        "usage: tpcc load <warehouses>\n"
		        "       tpcc run <warehouses> <terminals> <seconds> [--think]\n"
		        "       tpcc sweep <warehouses> <seconds>\n");
		return 2;
	}

	// Handles are shared between terminals, so SQLite has to be in its serialized mode. The
	// program used to spawn threads with no configuration call at all and no mutex anywhere,
	// which is the other half of the heap corruption: even with one owner for each database,
	// a library built for single-thread use will corrupt its own allocator.
	if (!sqlite3_threadsafe()) {
		fprintf(stderr, "this SQLite was built without thread safety; terminals would corrupt it\n");
		return 1;
	}
	sqlite3_config(SQLITE_CONFIG_SERIALIZED);

	if (weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"))) {
		fprintf(stderr, "FoundationDB did not start\n");
		return 1;
	}
	weft_vfs_register(0);

	// The two NURand C values for C_LAST must differ between load and run, 2.1.6.1, and the
	// difference is constrained. Fixed here rather than drawn, so a run is repeatable.
	C_LAST_LOAD = 137;
	C_LAST_RUN = 44;
	C_ID_CONST = 613;
	OL_I_ID_CONST = 3401;

	int warehouses = atoi(argv[2]);
	if (warehouses < 1) {
		fprintf(stderr, "warehouses must be at least 1\n");
		return 2;
	}

	if (strcmp(argv[1], "load") == 0) {
		struct rng r = { 0x243f6a8885a308d3ULL };
		double t0 = now();
		printf("loading %d items\n", ITEMS);
		if (load_items(&r)) return 1;
		for (int w = 1; w <= warehouses; w++) {
			printf("loading warehouse %d of %d\n", w, warehouses);
			if (load_warehouse(w, &r)) return 1;
		}
		printf("loaded in %.1fs\n", now() - t0);
		weft_fdb_stop();
		return 0;
	}

	if (strcmp(argv[1], "run") == 0) {
		if (argc < 5) {
			fprintf(stderr, "run needs <warehouses> <terminals> <seconds>\n");
			return 2;
		}
		int terminals = atoi(argv[3]);
		double seconds = atof(argv[4]);
		int think = argc > 5 && strcmp(argv[5], "--think") == 0;
		struct totals tot = { 0 };
		double elapsed = run_once(warehouses, terminals, seconds, think, &tot);
		if (elapsed < 0) return 1;
		report(&tot, elapsed, warehouses, terminals,
		       think ? "TPC-C with keying and think times" : "TPC-C at full rate");
		free_totals(&tot);
		weft_fdb_stop();
		return 0;
	}

	// The sweep is the point of the exercise: one number is a datum, and a curve is a
	// pattern. Terminals double until they pass the warehouse count, so the run covers both
	// sides of the one-owner boundary — below it every terminal has a database to itself,
	// above it they share.
	if (strcmp(argv[1], "sweep") == 0) {
		double seconds = argc > 3 ? atof(argv[3]) : 30.0;
		printf("%-10s %10s %12s %10s %10s\n", "terminals", "tpmC", "new-order/s", "p99 ms",
		       "groups");
		for (int terminals = 1; terminals <= warehouses * 4; terminals *= 2) {
			struct totals tot = { 0 };
			double elapsed = run_once(warehouses, terminals, seconds, 0, &tot);
			if (elapsed < 0) return 1;
			qsort(tot.lat[NEW_ORDER], (size_t)tot.lat_n[NEW_ORDER], sizeof(double),
			      cmp_double);
			printf("%-10d %10.1f %12.1f %10.2f %10lld\n", terminals,
			       (double)tot.count[NEW_ORDER] / elapsed * 60.0,
			       (double)tot.count[NEW_ORDER] / elapsed,
			       pct(tot.lat[NEW_ORDER], tot.lat_n[NEW_ORDER], 0.99) * 1000.0,
			       tot.remote_groups);
			fflush(stdout);
			free_totals(&tot);
		}
		weft_fdb_stop();
		return 0;
	}

	fprintf(stderr, "unknown mode: %s\n", argv[1]);
	return 2;
}
