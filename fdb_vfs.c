// A SQLite VFS whose files live in FoundationDB.
//
// This is the store described in README.md, beside this file. There is no
// local file, so an actor's database moves between machines with no copy and no restore
// step.
//
// Why a VFS and not a key and value table. A VFS gives SQLite its pages one page at a
// time, so SQLite reads the pages a query touches and no others. Three things follow, and
// none of them is true of the Elixir prototype this replaces:
//
//   - An actor is not limited by memory. The working set is in memory and the rest is in
//     FoundationDB, which is what makes the 10 GiB limit in `Weft.Limits` possible.
//   - A handoff copies nothing. A different machine opens the same database and reads
//     pages. There is no restore step and no transfer, so a large actor moves as fast as
//     a small one. `prove_handoff.c` is that property on its own.
//   - Compaction is not weft's to get wrong. The prototype's replicator folds a log by
//     hand and has a race. The layout below moves that work into one place with one
//     owner, and `Store.lean` proves the rule the fold must obey.
//
// Layout. The actor-supervisor Depot layout, modelled in docs/spec/Store.lean:
//
//   weft/db/<name>/HEAD                 the txid of the newest commit
//   weft/db/<name>/SIZE                 the file size, 8 bytes big endian
//   weft/db/<name>/FENCE                the ownership fence
//   weft/db/<name>/PIDX/<pgno>          the txid that owns a page
//   weft/db/<name>/DELTA/<txid>/<pgno>  the pages of one commit
//   weft/db/<name>/SHARD/<as_of>/<pgno> a compacted base, versioned by as_of
//   weft/db/<name>/SHARDN/<as_of>       the page count of a shard version
//   weft/db/<name>/LOGN                 the page count of the log since compaction
//
// A read finds the owner in PIDX and then reads one of DELTA or SHARD. So a read touches
// two rows whatever the log holds, which `Store.lean` proves as `read_touches_two_rows`.
//
// A commit is one FoundationDB transaction. The VFS holds the pages SQLite writes in
// memory, and it sends them when SQLite syncs the file. The pages of a commit go under
// one txid first, where no read can see them. One transaction then points PIDX at that
// txid and advances the head. A process that dies part way through leaves either the
// whole commit or none of it.
//
// Losing the commit that was in flight is correct. `Weft.Actor.Store` accepts that a
// crash loses the last few commits. Leaving pages from two commits is a different
// failure, because a reader cannot see it and it loses the whole actor.
//
// What a caller must set. Two pragmas, and neither is a tuning knob.
//
// `PRAGMA journal_mode=MEMORY`. The commit above is atomic, so a rollback journal on disk
// adds cost and protects nothing.
//
// `PRAGMA locking_mode=EXCLUSIVE`. Without it SQLite reads page 1 to check the change
// counter at the start of every read transaction, and over a database on the network that
// check is a round trip for every query. The pragma tells SQLite that nothing else can
// change the file, so it trusts its page cache and stops the re-read. An actor is the
// single writer of its own store, so the statement is true. It was worth more than the
// layout of the pages; weft's `docs/logbook/store.md` holds the number.
//
// A reader is the exception. A handle opened read-only keeps NORMAL locking, so SQLite
// re-reads page 1 at each transaction, finds the head where the change counter would be,
// and drops its cache when a commit has moved it. See `check_view`.
//
// Every transaction runs in the retry loop that FoundationDB documents.
// `fdb_transaction_on_error` decides if an error may be retried, and waits before the
// next attempt. A fence mismatch is not retried, because refusing the write is the
// correct answer.
//
// Locking is a no-op. An actor is the single writer of its own store. The fence, not a
// lock, is what stops a second writer. See the Weft moduledoc.
//
// How to read this file. It has four layers, and each one uses only the layer above it:
//
//   1. Keys.        `key_pidx` and the rest build one key each. A number goes into a key
//                   big endian, so the order of the keys is the order of the numbers.
//   2. Transactions. `run_txn` runs a body and retries it the way FoundationDB asks. Each
//                   body is a small function named `*_body`, and it reads and writes
//                   through the helpers above it. A body can run more than once, so it
//                   must build its own state every time.
//   3. Pages.       `page_from_store` reads one page, and the dirty buffer holds the
//                   pages SQLite wrote but has not committed.
//   4. SQLite.      `fdb_read`, `fdb_write`, `fdb_sync` and the rest are what SQLite
//                   calls. `flush` is where a commit happens.

#define _POSIX_C_SOURCE 200809L

#include "fdb_keys.h"

#include <fdb_c.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// SQLite's page size. This is not a choice.
#define PAGE 4096
#define MAX_NAME 512

// FoundationDB caps a value and a transaction. These are not choices either. Every
// limit below comes from them, so there is no constant to tune. See `Store.lean`.
#define FDB_TXN_LIMIT 10000000

// How much of a transaction one page costs, counting the key and not only the bytes. A
// limit that counts the pages alone overruns the transaction on the keys.
#define DELTA_ROW (PAGE + KEYMAX)
#define PIDX_ROW (KEYMAX + 8)

// The largest commit that goes in one transaction. Such a commit carries the page and
// its index row, so it pays for both. `Store.lean` derives the same bound as
// `maxCommitPages`.
#define ONE_TXN_PAGES (FDB_TXN_LIMIT / (DELTA_ROW + PIDX_ROW))

// The largest number of pages that one staging transaction carries. Staging writes the
// pages alone, so it pays for the page rows only.
#define STAGE_TXN_PAGES (FDB_TXN_LIMIT / DELTA_ROW)

// The largest number of index rows that fit the transaction that moves the head. This is
// what bounds a commit overall, because the index must land at once.
//
// It used to be `FDB_TXN_LIMIT / PIDX_ROW`, charging KEYMAX for every row. KEYMAX is the
// size of the *buffer* a key is built in, not the size of a key: `key_pidx` writes
// "weft/db/<name>/PIDX/" and four big-endian bytes, so a row is 18 + strlen(name) + 8. For
// a name of ten characters that is 36 bytes against the 648 that were charged, and the
// ceiling on an atomic commit was 18 times lower than the format requires — 63 MB where
// 1.1 GB fits. TPC-C found it: a warehouse is about 60 MB of rows and the load failed with
// SQLITE_IOERR_WRITE, which SQLite reports as "disk I/O error" while naming no limit.
//
// The budget is now the real one, computed once per file because the name does not change
// and the page number is always four bytes. `MAX_TXN_PIDX` stays as the floor a name of the
// greatest possible length would give, so a caller that wants one number still has one.
#define MAX_TXN_PIDX (FDB_TXN_LIMIT / PIDX_ROW)

// What else rides in the transaction that moves the head: the SIZE, LOGN and HEAD meta rows,
// the two keys of a truncate's clear range, and the fence read's conflict range. Each is a
// key of at most KEYMAX with an eight-byte value, so six of them is a generous reserve, and
// it is four thousandths of the limit either way.
#define HEAD_TXN_RESERVE (6 * (KEYMAX + 8))

static FDBDatabase *g_db;
static pthread_t g_network_thread;
static int g_started;

void weft_fdb_stop(void);

// ── Fault injection ───────────────────────────────────────────────────────────
//
// `WEFT_CRASH_AT_COMMIT=N` stops the process before its Nth write transaction commits.
// A crash point must be repeatable, because `prove_crash` searches over crash points and
// a search cannot repeat a failure it cannot address. The hook is inert when the
// variable is not set.

static long g_crash_at;
static long g_commits;

// `WEFT_PLANT_STALE_VIEW=1` plants the defect `check_view` fixes: a read transaction that
// checks nothing. `prove_fence_read` runs under it to show the fixture fails on the broken
// VFS, because a check that passes on known-broken input certifies the defect. Inert when
// the variable is not set.
static int g_plant_stale_view = -1;
static int plant_stale_view(void) {
	if (g_plant_stale_view < 0) {
		const char *v = getenv("WEFT_PLANT_STALE_VIEW");
		g_plant_stale_view = v && *v && strcmp(v, "0") != 0;
	}
	return g_plant_stale_view;
}

// `WEFT_PLANT_TORN_COMMIT=1` plants the defect a one-transaction commit prevents: each page
// becomes visible in its own transaction, so a crash between two leaves a torn commit. The
// crash attacks run under it to show they can find a tear. Inert when the variable is not set.
static int g_plant_torn_commit = -1;
static int plant_torn_commit(void) {
	if (g_plant_torn_commit < 0) {
		const char *v = getenv("WEFT_PLANT_TORN_COMMIT");
		g_plant_torn_commit = v && *v && strcmp(v, "0") != 0;
	}
	return g_plant_torn_commit;
}

// The page-delta plants, one for each way `spec/PageDelta.lean` shows a writer can break
// the rule that a delta's base is the full row it names. Each is inert unless its variable
// is set. CHAIN takes a delta row as a later delta's base, STALE keeps a base's name but not
// its bytes, FOLD keeps bases across a fold that deleted their rows.
static int plant_flag(int *cache, const char *name) {
	if (*cache < 0) {
		const char *v = getenv(name);
		*cache = v && *v && strcmp(v, "0") != 0;
	}
	return *cache;
}
static int g_plant_delta_chain = -1, g_plant_delta_stale = -1, g_plant_delta_fold = -1;
static int plant_delta_chain(void) {
	return plant_flag(&g_plant_delta_chain, "WEFT_PLANT_DELTA_CHAIN");
}
static int plant_delta_stale(void) {
	return plant_flag(&g_plant_delta_stale, "WEFT_PLANT_DELTA_STALE");
}
static int plant_delta_fold(void) {
	return plant_flag(&g_plant_delta_fold, "WEFT_PLANT_DELTA_FOLD");
}

// `WEFT_PLANT_SLOW_COMMIT=1` plants a performance defect: each commit pays one extra read
// round trip first. The timing attacks run under it to show they can see a slow commit.
// Inert when the variable is not set.
static int g_plant_slow_commit = -1;
static int plant_slow_commit(void) {
	if (g_plant_slow_commit < 0) {
		const char *v = getenv("WEFT_PLANT_SLOW_COMMIT");
		g_plant_slow_commit = v && *v && strcmp(v, "0") != 0;
	}
	return g_plant_slow_commit;
}

static void crash_point(void) {
	if (g_crash_at && ++g_commits == g_crash_at) {
		// _exit, so nothing is flushed and no handler runs. This is the crash a machine
		// failure gives.
		_exit(9);
	}
}

// ── FoundationDB helpers ──────────────────────────────────────────────────────

static void *run_network(void *unused) {
	(void)unused;
	fdb_run_network();
	return NULL;
}

// Start the client once for the process. `cluster_file` may be NULL for the default.
int weft_fdb_start(const char *cluster_file) {
	const char *at = getenv("WEFT_CRASH_AT_COMMIT");
	g_crash_at = at ? atol(at) : 0;

	fdb_error_t err = fdb_select_api_version(FDB_API_VERSION);
	if (err) return err;
	if ((err = fdb_setup_network())) return err;
	if (pthread_create(&g_network_thread, NULL, run_network, NULL)) return -1;
	g_started = 1;

	// Stop on the way out, however the caller leaves.
	//
	// The network thread outlives a `return` from main, and a process that exits while it
	// is still running crashes. Every program here has error paths that return without
	// stopping, so a refused open reported itself as a segfault — a harness hiding the
	// failure it exists to show. Registering it once fixes all of them, including ones not
	// written yet.
	atexit(weft_fdb_stop);
	return fdb_create_database(cluster_file, &g_db);
}

// Idempotent, because it is now called both by hand and at exit, and joining a thread twice
// is not a thing to leave to luck.
void weft_fdb_stop(void) {
	if (!g_started) return;
	g_started = 0;
	if (g_db) {
		fdb_database_destroy(g_db);
		g_db = NULL;
	}
	fdb_stop_network();
	pthread_join(g_network_thread, NULL);
}

// Block the calling thread until a future is ready. The store runs its own
// threads, so blocking here does not touch a BEAM scheduler.
static fdb_error_t await(FDBFuture *f) {
	fdb_error_t err = fdb_future_block_until_ready(f);
	if (!err) err = fdb_future_get_error(f);
	return err;
}

// ── Keys ──────────────────────────────────────────────────────────────────────
//
// In `fdb_keys.h`, because they depend on neither FoundationDB nor SQLite and
// `keys-witness/keys_test.cc` tests them without either. A number goes into a key big endian, so
// the order of the keys is the order of the numbers, and a range read then gives pages
// and commits in order.

// ── The transaction runner ────────────────────────────────────────────────────

// A transaction body. It returns a FoundationDB error, or zero. It sets `final` to a
// SQLite code when the failure must reach the caller instead of being retried.
//
// A body runs again after a retryable error, so a body must build its own state and must
// not depend on what an earlier attempt did.
typedef fdb_error_t (*txn_body)(FDBTransaction *tr, void *ctx, int *final);

static int run_txn(txn_body body, void *ctx, int is_write, int ioerr) {
	FDBTransaction *tr = NULL;
	if (fdb_database_create_transaction(g_db, &tr)) return ioerr;

	for (;;) {
		int final = 0;
		fdb_error_t err = body(tr, ctx, &final);
		if (final) {
			fdb_transaction_destroy(tr);
			return final;
		}
		if (!err) {
			if (!is_write) {
				// A read needs no commit. Ending it here releases the read version.
				fdb_transaction_destroy(tr);
				return SQLITE_OK;
			}
			crash_point();
			FDBFuture *c = fdb_transaction_commit(tr);
			err = await(c);
			fdb_future_destroy(c);
			if (!err) {
				fdb_transaction_destroy(tr);
				return SQLITE_OK;
			}
		}

		// FoundationDB decides whether the error may be retried, and waits the right
		// amount before the next attempt. It resets the transaction when it may be
		// retried, and it returns the error when it may not.
		FDBFuture *r = fdb_transaction_on_error(tr, err);
		fdb_error_t fatal = await(r);
		fdb_future_destroy(r);
		if (fatal) {
			fdb_transaction_destroy(tr);
			return ioerr;
		}
	}
}

// ── Reads inside a transaction ────────────────────────────────────────────────

static fdb_error_t get_bytes(FDBTransaction *tr, const uint8_t *key, int klen, uint8_t *out,
                             int max, int *len, int *present) {
	FDBFuture *f = fdb_transaction_get(tr, key, klen, 0);
	fdb_error_t err = await(f);
	if (!err) {
		fdb_bool_t got;
		const uint8_t *val;
		int vlen;
		err = fdb_future_get_value(f, &got, &val, &vlen);
		if (!err) {
			*present = got;
			if (got) {
				int n = vlen > max ? max : vlen;
				memcpy(out, val, (size_t)n);
				if (len) *len = n;
			}
		}
	}
	fdb_future_destroy(f);
	return err;
}

static fdb_error_t get_u64(FDBTransaction *tr, const uint8_t *key, int klen, uint64_t *out,
                           int *present) {
	uint8_t val[8];
	int len = 0, got = 0;
	fdb_error_t err = get_bytes(tr, key, klen, val, 8, &len, &got);
	if (err) return err;
	*present = got;
	if (got && len == 8) *out = get_be64(val);
	else *present = 0;
	return 0;
}

static void set_u64(FDBTransaction *tr, const uint8_t *key, int klen, uint64_t v) {
	uint8_t val[8];
	put_be64(val, v);
	fdb_transaction_set(tr, key, klen, val, 8);
}

// Read the number at the end of the first or the last key of a range. Every versioned
// key in this layout ends with its number, so this is how the VFS asks for the newest
// shard version or the oldest pin. `found` stays 0 when the range is empty.
static fdb_error_t edge_number(FDBTransaction *tr, const uint8_t *begin, int blen,
                               const uint8_t *end, int elen, int want_last, uint64_t *out,
                               int *found) {
	*found = 0;
	FDBFuture *f = fdb_transaction_get_range(tr, FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(begin, blen),
	                                         FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(end, elen), 1, 0,
	                                         FDB_STREAMING_MODE_WANT_ALL, 1, 0, want_last);
	fdb_error_t err = await(f);
	if (!err) {
		const FDBKeyValue *kv;
		int count;
		fdb_bool_t more;
		err = fdb_future_get_keyvalue_array(f, &kv, &count, &more);
		if (!err && count > 0 && kv[0].key_length >= 8) {
			*out = get_be64(kv[0].key + kv[0].key_length - 8);
			*found = 1;
		}
	}
	fdb_future_destroy(f);
	return err;
}

// Clear every key under `weft/db/<name>/<what>/`. FoundationDB stores a range clear as
// one small record, so the cost does not grow with the number of keys it removes.
static void clear_prefix(FDBTransaction *tr, const char *name, const char *what) {
	uint8_t from[KEYMAX], to[KEYMAX];
	int flen = key_prefix(from, name, what);
	int tlen = key_after(to, from, flen);
	fdb_transaction_clear_range(tr, from, flen, to, tlen);
}

// ── The file ──────────────────────────────────────────────────────────────────

// One page that SQLite wrote and the store has not taken yet.
typedef struct {
	uint32_t pgno;
	uint8_t as_delta; // the commit sent this page as a delta, set while the commit is built
	uint8_t bytes[PAGE];
} DirtyPage;

// The full row a page's next delta is taken against: a log row by txid, or a shard page by
// its version. `bytes` is that row as stored, so a delta of `bytes` applies to the row.
#define BASE_TXID 1
#define BASE_SHARD 2
#define BASE_SLOTS 4096
typedef struct {
	uint32_t pgno;
	uint8_t kind; // 0 for an empty slot
	uint64_t id;
	uint8_t bytes[PAGE];
} BaseEntry;

typedef struct {
	sqlite3_file base;
	char name[MAX_NAME];
	int64_t fence;
	int readonly; // opened with SQLITE_OPEN_READONLY, so no fence: the view is HEAD and the shard
	// SQLite holds no page it will use before re-reading the header: set at open and at
	// every unlock to none, cleared by every read. Only `check_view` reads it.
	int cache_droppable;

	uint64_t head;        // the newest commit
	uint64_t shard_as_of; // the newest shard version
	int has_shard;

	// The page counts that decide when to compact. The single writer owns both, so it
	// keeps them here and does not read them back. A round trip for a number this
	// process already knows is the most expensive way to learn nothing.
	uint64_t base_pages; // the pages of the newest shard version
	uint64_t log_pages;  // the pages committed since that version

	// The group this file joined, or zero. While it is set, a sync stages instead of
	// committing, because the group's record is what makes the write real.
	uint64_t group_txnid;

	// The log has reached the base and a fold is owed. It is a flag rather than a call,
	// because the commit that notices is the worst moment to do the work.
	int compact_due;

	// Read-ahead. `ra_score` is how much this file looks like a scan, `ra_last` is the page
	// the score was last moved by, and the window is what a prefetch brought back.
	int ra_score;
	uint32_t ra_last;
	uint8_t *ra_window; // ra_count pages, or NULL until a scan earns one
	// What the prefetch learned about each page in the window. Absent and unfetched have
	// to be different answers: a page the prefetch chose to skip still exists, and reading
	// it as a hole would hand SQLite a zeroed page.
	uint8_t *ra_state; // RA_UNFETCHED, RA_PRESENT or RA_ABSENT
	uint32_t ra_first;
	uint32_t ra_count;

	int64_t size;      // the file size, counting what is buffered
	int64_t sent_size; // the file size the store holds

	// The pages SQLite wrote, sorted by page number. They are the truth for a read
	// until a sync sends them.
	DirtyPage **dirty;
	int ndirty, capdirty;

	// The page count at or above which every page is dropped by the pending truncate.
	// -1 means no truncate is pending.
	int64_t trunc_pages;

	// What one index row of this file costs the transaction that moves the head: the key
	// `key_pidx` builds for it, plus the eight-byte value. Fixed for the life of the file,
	// since the name does not change and a page number is always four bytes.
	int pidx_row;

	// Cached read transaction, reused across xRead calls to spread one GRV over
	// many page reads. FDB caps a transaction's lifetime at five seconds; we
	// hold ours for four so the deadline never surprises us mid-read. Every
	// `flush` (a successful FDB commit) invalidates it, so a reader never sees
	// bytes older than this connection's own last write.
	FDBTransaction *ro_tr;
	uint64_t ro_tr_deadline_us;

	// Bases for page deltas, one slot per page number modulo BASE_SLOTS, allocated by the
	// first read of a writer. A missing base costs a full page, never a wrong one.
	BaseEntry *bases;
} FdbFile;

// A read that reuses a cached FDB transaction across xRead calls on the same
// file. One SQLite BEGIN..COMMIT can issue thousands of page reads; the store's
// pre-change shape spent a fresh GRV round-trip on each. The cache holds one
// FDBTransaction on the FdbFile, refreshing when either FDB's five-second
// transaction lifetime is about to expire or a commit has moved the head. A
// retryable error drops the cached tr and falls through to a fresh transaction,
// same as `run_txn`.
static fdb_error_t check_view(FDBTransaction *tr, FdbFile *f, int *final);

static int run_read_txn(txn_body body, void *ctx, FdbFile *f) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	const uint64_t now = (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;

	if (f->ro_tr && now < f->ro_tr_deadline_us) {
		int final = 0;
		fdb_error_t err = body(f->ro_tr, ctx, &final);
		if (final) return final;
		if (!err) return SQLITE_OK;
		fdb_transaction_destroy(f->ro_tr);
		f->ro_tr = NULL;
	}

	FDBTransaction *tr = NULL;
	if (fdb_database_create_transaction(g_db, &tr)) return SQLITE_IOERR_READ;

	for (;;) {
		int final = 0;
		// The view is checked once per transaction, not once per read: every read in it is
		// at the one read version the check saw.
		fdb_error_t err = check_view(tr, f, &final);
		if (!err && !final) err = body(tr, ctx, &final);
		if (final) {
			fdb_transaction_destroy(tr);
			return final;
		}
		if (!err) {
			if (f->ro_tr) fdb_transaction_destroy(f->ro_tr);
			f->ro_tr = tr;
			// FDB caps a transaction at five seconds; we hold ours for four
			// so a slow read never trips the cap mid-flight.
			f->ro_tr_deadline_us = now + 4000000ULL;
			return SQLITE_OK;
		}

		FDBFuture *r = fdb_transaction_on_error(tr, err);
		fdb_error_t fatal = await(r);
		fdb_future_destroy(r);
		if (fatal) {
			fdb_transaction_destroy(tr);
			return SQLITE_IOERR_READ;
		}
	}
}


// Find the slot of `pgno`, or where it would go. Returns 1 when it is there.
static int find_dirty(FdbFile *f, uint32_t pgno, int *slot) {
	int lo = 0, hi = f->ndirty;
	while (lo < hi) {
		int mid = lo + (hi - lo) / 2;
		if (f->dirty[mid]->pgno < pgno) lo = mid + 1;
		else hi = mid;
	}
	*slot = lo;
	return lo < f->ndirty && f->dirty[lo]->pgno == pgno;
}

static DirtyPage *insert_dirty(FdbFile *f, uint32_t pgno, int slot) {
	if (f->ndirty == f->capdirty) {
		int cap = f->capdirty ? f->capdirty * 2 : 32;
		DirtyPage **grown = realloc(f->dirty, (size_t)cap * sizeof(*grown));
		if (!grown) return NULL;
		f->dirty = grown;
		f->capdirty = cap;
	}
	DirtyPage *p = calloc(1, sizeof(*p));
	if (!p) return NULL;
	p->pgno = pgno;
	memmove(&f->dirty[slot + 1], &f->dirty[slot],
	        (size_t)(f->ndirty - slot) * sizeof(*f->dirty));
	f->dirty[slot] = p;
	f->ndirty++;
	return p;
}

static void clear_dirty(FdbFile *f) {
	for (int i = 0; i < f->ndirty; i++) free(f->dirty[i]);
	f->ndirty = 0;
	f->trunc_pages = -1;
}

// ── Read-ahead ────────────────────────────────────────────────────────────────
//
// A page miss is a network round trip, so a VFS over FoundationDB lives or dies here. A
// cold scan of a thousand pages was a thousand round trips, and `docs/spec/Prefetch.lean`
// models the way out.
//
// It does not pick a depth. It scores the access pattern, and the numbers below are that
// model's, not ones chosen here:
//
//   forward page, gap of 8 or less   score + 2, capped at 12
//   random page, a scan is running   score - 1
//   random page, no scan             score - 4
//   score of 6 or more               escalate read-ahead
//   score of 10 or more              full depth, 256 pages or 1 MB
//
// Up 2 and down 4 means a scan must be twice as consistent as the noise to hold its
// credit. The softer decay while a scan is already running tolerates the interleaving a
// real B-tree walk gives. So a point read pays for no read-ahead, and a table scan
// escalates after three pages, and neither is configured.
//
// What makes the prefetch cheap is the layout. PIDX is keyed by page number, so the owners
// of a run of pages are one contiguous range read. A compacted database keeps its pages in
// SHARD, also keyed by page number, so those are a second contiguous range read. A scan
// over a folded database therefore costs two round trips for every 256 pages instead of
// 256. Pages still in the log are scattered by txid and are left to the ordinary path,
// which is why compaction is what makes a scan fast rather than read-ahead alone.

#define RA_GAP 8
#define RA_SCORE_STEP 2
#define RA_SCORE_CAP 12
#define RA_DECAY_SCANNING 1
#define RA_DECAY_RANDOM 4
#define RA_ESCALATE 6
#define RA_FULL 10

// Full depth is 256 pages or 1 MB, whichever is smaller, which at a 4 KiB page is both.
#define RA_MAX_PAGES 256
#define RA_STEP_PAGES 32

// A prefetch does not fetch every page it covers, so the window has to say which of the
// three it means. Skipping this distinction reads a log-owned page as a hole.
#define RA_UNFETCHED 0
#define RA_PRESENT 1
#define RA_ABSENT 2

// Move the score by what this page number says about the pattern, and answer how many
// pages are worth fetching ahead.
static uint32_t ra_depth(FdbFile *f, uint32_t pgno) {
	const int64_t gap = (int64_t)pgno - (int64_t)f->ra_last;

	if (gap > 0 && gap <= RA_GAP) {
		f->ra_score += RA_SCORE_STEP;
		if (f->ra_score > RA_SCORE_CAP) f->ra_score = RA_SCORE_CAP;
	} else if (f->ra_score >= RA_ESCALATE) {
		f->ra_score -= RA_DECAY_SCANNING;
	} else {
		f->ra_score -= RA_DECAY_RANDOM;
		if (f->ra_score < 0) f->ra_score = 0;
	}
	f->ra_last = pgno;

	if (f->ra_score >= RA_FULL) return RA_MAX_PAGES;
	if (f->ra_score >= RA_ESCALATE) return RA_STEP_PAGES;
	return 0;
}

// Is this page already in the window a prefetch brought back?
static int ra_hit(FdbFile *f, uint32_t pgno, uint8_t *out, int *len, int *present) {
	if (!f->ra_window || pgno < f->ra_first || pgno >= f->ra_first + f->ra_count) return 0;
	const uint32_t i = pgno - f->ra_first;

	if (f->ra_state[i] == RA_ABSENT) {
		// The prefetch looked and there was nothing there, which is as good an answer as a
		// page and saves the same round trip.
		*present = 0;
		*len = 0;
		return 1;
	}
	if (f->ra_state[i] != RA_PRESENT) return 0; // never fetched, so this says nothing

	memcpy(out, f->ra_window + (size_t)i * PAGE, PAGE);
	*len = PAGE;
	*present = 1;
	return 1;
}

// Read a run of pages in as few round trips as the layout allows.
//
// Anything this cannot place is simply left out of the window, and the ordinary path picks
// it up one page at a time. A prefetch that guesses wrong costs a range read, never a
// wrong answer.
static fdb_error_t ra_fill(FDBTransaction *tr, FdbFile *f, uint32_t first, uint32_t count) {
	if (!f->ra_window) {
		f->ra_window = malloc((size_t)RA_MAX_PAGES * PAGE);
		f->ra_state = malloc(RA_MAX_PAGES);
		if (!f->ra_window || !f->ra_state) {
			free(f->ra_window);
			free(f->ra_state);
			f->ra_window = NULL;
			f->ra_state = NULL;
			return 0; // no window, so no read-ahead; the ordinary path still answers
		}
	}
	if (count > RA_MAX_PAGES) count = RA_MAX_PAGES;
	memset(f->ra_state, RA_UNFETCHED, count);
	f->ra_first = first;
	f->ra_count = count;

	uint8_t from[KEYMAX], to[KEYMAX];

	// How far the reads below actually reached.
	//
	// A range read answers with what fitted and says `more` when it stopped early, and a
	// window of 256 pages is a megabyte, which is the size that gets stopped. A page the
	// read never reached is one nothing is known about, and `spec/ReadAhead.lean` proves
	// that calling it absent loses it: `ignoring_more_loses_a_page`.
	//
	// So coverage is tracked and the window shrinks to it. A page past the end is then
	// outside the window entirely, which the same file proves is always safe.
	uint32_t covered_to = first + count;

	// Which txid owns each page, as one range read. A page with no row here comes from the
	// shard, which is the common case for a folded database.
	uint64_t *owner = calloc(count, sizeof(uint64_t));
	if (!owner) return 0;

	int flen = key_pidx(from, f->name, first);
	int tlen = key_pidx(to, f->name, first + count);
	FDBFuture *fu = fdb_transaction_get_range(tr, FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(from, flen),
	                                          FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(to, tlen), 0, 0,
	                                          FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);
	fdb_error_t err = await(fu);
	if (!err) {
		const FDBKeyValue *kv;
		int n;
		fdb_bool_t more;
		err = fdb_future_get_keyvalue_array(fu, &kv, &n, &more);
		if (!err) {
			uint32_t reached = first;
			for (int i = 0; i < n; i++) {
				if (kv[i].key_length < 4 || kv[i].value_length != 8) continue;
				const uint8_t *p = kv[i].key + kv[i].key_length - 4;
				uint32_t page = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
				                | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
				if (page >= first && page < first + count) {
					owner[page - first] = get_be64(kv[i].value);
					if (page + 1 > reached) reached = page + 1;
				}
			}
			// PIDX is sparse, so a short answer is ordinary and says nothing on its own.
			// Only `more` means the read stopped before the end of the range.
			if (more && reached < covered_to) covered_to = reached;
		}
	}
	fdb_future_destroy(fu);
	if (err) {
		free(owner);
		return err;
	}

	// The pages the shard holds, as a second range read. SHARD is keyed by page number
	// under one version, so a run of pages is one contiguous range.
	if (f->has_shard) {
		flen = key_shard(from, f->name, f->shard_as_of, first);
		tlen = key_shard(to, f->name, f->shard_as_of, first + count);
		fu = fdb_transaction_get_range(tr, FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(from, flen),
		                               FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(to, tlen), 0, 0,
		                               FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);
		err = await(fu);
		if (!err) {
			const FDBKeyValue *kv;
			int n;
			fdb_bool_t more;
			err = fdb_future_get_keyvalue_array(fu, &kv, &n, &more);
			if (!err) {
				uint32_t reached = first;
				for (int i = 0; i < n; i++) {
					if (kv[i].key_length < 4 || kv[i].value_length > PAGE) continue;
					const uint8_t *p = kv[i].key + kv[i].key_length - 4;
					uint32_t page = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
					                | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
					if (page < first || page >= first + count) continue;
					const uint32_t j = page - first;
					// A page the log owns is newer than the shard's copy, so it is not this.
					if (owner[j]) continue;
					memset(f->ra_window + (size_t)j * PAGE, 0, PAGE);
					memcpy(f->ra_window + (size_t)j * PAGE, kv[i].value,
					       (size_t)kv[i].value_length);
					f->ra_state[j] = RA_PRESENT;
					if (page + 1 > reached) reached = page + 1;
				}
				if (more && reached < covered_to) covered_to = reached;
			}
		}
		fdb_future_destroy(fu);
	}

	// A page whose owner is a txid stays unfetched: DELTA is keyed by txid first, so those
	// are scattered and are not one range. The ordinary path reads them, and compaction is
	// what moves them into the shard where a scan can reach them cheaply.
	//
	// A page with no owner and nothing in the shard is genuinely a hole, and saying so is
	// worth as much as a page: it saves the same round trip. That is only true because the
	// range read above covered it, which is why the two cases cannot share a flag.
	for (uint32_t i = 0; i < count; i++) {
		if (owner[i]) f->ra_state[i] = RA_UNFETCHED;
		else if (f->ra_state[i] != RA_PRESENT) f->ra_state[i] = f->has_shard ? RA_ABSENT
		                                                                    : RA_UNFETCHED;
	}

	// Shrink to what was actually read. Everything past this was never looked at, and the
	// ordinary path answers for it one page at a time.
	f->ra_count = covered_to > first ? covered_to - first : 0;

	free(owner);
	return err;
}

// Throw the window away.
//
// A window survives the transaction that filled it, so anything that changes what a page
// holds, or where it lives, must drop it. The fence stops another process from being that
// thing, so it is only ever this one: a commit, a fold, or a truncate.
static void ra_reset(FdbFile *f) { f->ra_count = 0; }

// ── Page deltas ───────────────────────────────────────────────────────────────
//
// A page is stored as its difference from a full row when under a fifth of it changed.
// `spec/PageDelta.lean` models the rule: the base of a delta is always a full row, so a
// read touches at most two rows. A full row is exactly PAGE bytes; a delta is shorter and
// starts with DELTA_MAGIC, then the base's kind and id, then runs of offset, length and
// bytes. The base comes from the writer's memory, so a commit adds no read.

#define DELTA_MAGIC 0xD7
#define DELTA_HDR 10
#define DELTA_CUT 5
#define DELTA_GAP 8

static int is_delta(const uint8_t *val, int vlen) {
	return vlen != PAGE && vlen >= DELTA_HDR && val[0] == DELTA_MAGIC;
}

static void put_be16(uint8_t *p, uint32_t v) {
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)v;
}

static uint32_t get_be16(const uint8_t *p) { return ((uint32_t)p[0] << 8) | p[1]; }

// The delta of `page` against `base`, or 0 when it would not be under the cut.
static int delta_encode(const uint8_t *base, const uint8_t *page, int kind, uint64_t id,
                        uint8_t *out) {
	const int limit = PAGE / DELTA_CUT;
	int n = DELTA_HDR;
	out[0] = DELTA_MAGIC;
	out[1] = (uint8_t)kind;
	for (int s = 0; s < 8; s++) out[2 + s] = (uint8_t)(id >> (56 - 8 * s));
	for (int i = 0; i < PAGE;) {
		if (base[i] == page[i]) {
			i++;
			continue;
		}
		int end = i + 1, same = 0;
		for (int j = i + 1; j < PAGE && same < DELTA_GAP; j++) {
			if (base[j] == page[j]) {
				same++;
			} else {
				same = 0;
				end = j + 1;
			}
		}
		const int len = end - i;
		if (n + 4 + len > limit) return 0;
		put_be16(out + n, (uint32_t)i);
		put_be16(out + n + 2, (uint32_t)len);
		memcpy(out + n + 4, page + i, (size_t)len);
		n += 4 + len;
		i = end;
	}
	return n;
}

static uint64_t delta_base_id(const uint8_t *val) {
	uint64_t id = 0;
	for (int s = 0; s < 8; s++) id = (id << 8) | val[2 + s];
	return id;
}

// Apply a delta to `page`, which holds its base. Returns 0, or -1 for a malformed delta.
static int delta_apply(const uint8_t *val, int vlen, uint8_t *page) {
	int n = DELTA_HDR;
	while (n < vlen) {
		if (n + 4 > vlen) return -1;
		const uint32_t off = get_be16(val + n), len = get_be16(val + n + 2);
		if (off + len > PAGE || n + 4 + (int)len > vlen) return -1;
		memcpy(page + off, val + n + 4, len);
		n += 4 + (int)len;
	}
	return 0;
}

static int base_key(uint8_t *key, FdbFile *f, int kind, uint64_t id, uint32_t pgno) {
	if (kind == BASE_TXID) return key_delta(key, f->name, id, pgno);
	if (kind == BASE_SHARD) return key_shard(key, f->name, id, pgno);
	return -1;
}

static BaseEntry *base_get(FdbFile *f, uint32_t pgno) {
	if (!f->bases) return NULL;
	BaseEntry *e = &f->bases[pgno % BASE_SLOTS];
	return e->kind && e->pgno == pgno ? e : NULL;
}

static void base_put(FdbFile *f, uint32_t pgno, int kind, uint64_t id, const uint8_t *bytes) {
	if (f->readonly) return;
	if (!f->bases) {
		f->bases = calloc(BASE_SLOTS, sizeof(BaseEntry));
		if (!f->bases) return;
	}
	BaseEntry *e = &f->bases[pgno % BASE_SLOTS];
	e->pgno = pgno;
	e->kind = (uint8_t)kind;
	e->id = id;
	memcpy(e->bytes, bytes, PAGE);
}

static void base_clear(FdbFile *f) {
	if (f->bases) memset(f->bases, 0, (size_t)BASE_SLOTS * sizeof(BaseEntry));
}

// Finish a row read into `out`: a full row is the page and becomes the page's base; a
// delta reads its base, which must be full, and applies to it.
static fdb_error_t finish_row(FDBTransaction *tr, FdbFile *f, uint32_t pgno, int kind,
                              uint64_t id, uint8_t *out, int *len) {
	if (!is_delta(out, *len)) {
		if (*len == PAGE) base_put(f, pgno, kind, id, out);
		return 0;
	}
	uint8_t d[PAGE];
	const int dlen = *len;
	memcpy(d, out, (size_t)dlen);
	uint8_t key[KEYMAX];
	const int bkind = d[1];
	const uint64_t bid = delta_base_id(d);
	const int klen = base_key(key, f, bkind, bid, pgno);
	if (klen < 0) return 2000; // FoundationDB client_invalid_operation: not retryable
	int blen = 0, have = 0;
	memset(out, 0, PAGE);
	fdb_error_t err = get_bytes(tr, key, klen, out, PAGE, &blen, &have);
	if (err) return err;
	if (!have || blen != PAGE) return 2000;
	base_put(f, pgno, bkind, bid, out);
	if (delta_apply(d, dlen, out) != 0) return 2000;
	*len = PAGE;
	return 0;
}

// Read one page from the store. `present` stays 0 when no commit and no shard holds it,
// and the caller then zero-fills.
//
// The read pipelines PIDX and DELTA(HEAD): both futures are issued at once, so the network
// carries them together. When PIDX names HEAD, the speculative DELTA future already holds
// the answer and one round trip is saved. When PIDX names a different txid or is empty,
// the speculative future is discarded and the ordinary second read is issued. Both keys
// go through the transaction unqualified, so both land in the FDB read-conflict set --
// `spec/PipelinedRead.sequential_read_set_subset_pipelined` is what carries that.
static fdb_error_t page_from_store(FDBTransaction *tr, FdbFile *f, uint32_t pgno,
                                   uint8_t *out, int *len, int *present) {
	uint8_t pidx_key[KEYMAX], data_key[KEYMAX];
	uint64_t owner = 0;
	int got = 0;

	*present = 0;
	*len = 0;

	// A window holds shard pages only, so a hit is a full shard row.
	if (ra_hit(f, pgno, out, len, present)) {
		if (*present) base_put(f, pgno, BASE_SHARD, f->shard_as_of, out);
		return 0;
	}

	const uint32_t depth = ra_depth(f, pgno);
	if (depth) {
		fdb_error_t pre = ra_fill(tr, f, pgno, depth);
		if (pre) return pre;
		if (ra_hit(f, pgno, out, len, present)) {
			if (*present) base_put(f, pgno, BASE_SHARD, f->shard_as_of, out);
			return 0;
		}
	}

	const int pidx_klen = key_pidx(pidx_key, f->name, pgno);
	FDBFuture *pidx_f = fdb_transaction_get(tr, pidx_key, pidx_klen, 0);

	// A fresh database has no commits, so there is no HEAD to speculate against. Skip the
	// speculative future in that case; the sequential path is exactly right.
	FDBFuture *spec_f = NULL;
	if (f->head) {
		int spec_klen = key_delta(data_key, f->name, f->head, pgno);
		spec_f = fdb_transaction_get(tr, data_key, spec_klen, 0);
	}

	fdb_error_t err = await(pidx_f);
	if (!err) {
		fdb_bool_t have;
		const uint8_t *val;
		int vlen;
		err = fdb_future_get_value(pidx_f, &have, &val, &vlen);
		if (!err && have && vlen == 8) {
			owner = get_be64(val);
			got = 1;
		}
	}
	fdb_future_destroy(pidx_f);
	if (err) {
		if (spec_f) fdb_future_destroy(spec_f);
		return err;
	}

	if (got && spec_f && owner == f->head) {
		// The speculation hit: the answer is already coming back.
		err = await(spec_f);
		if (!err) {
			fdb_bool_t have;
			const uint8_t *val;
			int vlen;
			err = fdb_future_get_value(spec_f, &have, &val, &vlen);
			if (!err) {
				*present = have;
				if (have) {
					int n = vlen > PAGE ? PAGE : vlen;
					memcpy(out, val, (size_t)n);
					*len = n;
				}
			}
		}
		fdb_future_destroy(spec_f);
		if (!err && *present) err = finish_row(tr, f, pgno, BASE_TXID, owner, out, len);
		return err;
	}

	if (spec_f) fdb_future_destroy(spec_f);

	if (got) {
		int klen = key_delta(data_key, f->name, owner, pgno);
		err = get_bytes(tr, data_key, klen, out, PAGE, len, present);
		if (!err && *present) err = finish_row(tr, f, pgno, BASE_TXID, owner, out, len);
		return err;
	}
	if (f->has_shard) {
		int klen = key_shard(data_key, f->name, f->shard_as_of, pgno);
		err = get_bytes(tr, data_key, klen, out, PAGE, len, present);
		if (!err && *present && *len == PAGE) base_put(f, pgno, BASE_SHARD, f->shard_as_of, out);
		return err;
	}
	return 0;
}

// ── Open ──────────────────────────────────────────────────────────────────────

struct open_ctx {
	FdbFile *f;
	int readonly;
};

// Where the database stands: the newest commit and the file size.
static fdb_error_t load_head(FDBTransaction *tr, FdbFile *file) {
	uint8_t key[KEYMAX];
	uint64_t size = 0;
	int got = 0;
	fdb_error_t err;

	int klen = key_meta(key, file->name, "HEAD");
	if ((err = get_u64(tr, key, klen, &file->head, &got))) return err;

	klen = key_meta(key, file->name, "SIZE");
	if ((err = get_u64(tr, key, klen, &size, &got))) return err;
	file->size = (int64_t)size;
	return 0;
}

// The newest shard version at or below the head. `Store.lean` calls this shardAt, and it
// is the base a read falls through to when no commit owns the page.
static fdb_error_t load_newest_shard(FDBTransaction *tr, FdbFile *file) {
	uint8_t from[KEYMAX], to[KEYMAX], key[KEYMAX];
	uint64_t as_of = 0;
	int found = 0;

	int flen = key_prefix(from, file->name, "SHARDN");
	int tlen = key_shardn(to, file->name, file->head + 1);
	fdb_error_t err = edge_number(tr, from, flen, to, tlen, 1, &as_of, &found);
	if (err) return err;

	file->has_shard = found;
	file->shard_as_of = found ? as_of : 0;

	// The sizes that decide when to compact are read once, here, and then kept in step
	// by the commit and by compaction.
	file->base_pages = 0;
	if (found) {
		int got = 0;
		int klen = key_shardn(key, file->name, as_of);
		if ((err = get_u64(tr, key, klen, &file->base_pages, &got))) return err;
		if (!got) file->base_pages = 0;
	}

	int got = 0;
	int klen = key_meta(key, file->name, "LOGN");
	if ((err = get_u64(tr, key, klen, &file->log_pages, &got))) return err;
	if (!got) file->log_pages = 0;
	return 0;
}

// Drop the pages of a commit that never finished.
//
// A commit writes its pages under a txid above the head before it advances the head. A
// process that died between the two leaves those pages behind. They are unreachable,
// because no PIDX row points at them, so clearing them frees space and changes no read.
static void drop_unfinished_commit(FDBTransaction *tr, FdbFile *file) {
	uint8_t from[KEYMAX], to[KEYMAX];
	int flen = key_delta_txid(from, file->name, file->head + 1);
	int plen = key_prefix(to, file->name, "DELTA");
	int tlen = key_after(to, to, plen);
	fdb_transaction_clear_range(tr, from, flen, to, tlen);
}

// Take ownership. A writer that opened earlier now holds a stale number, and its next
// write is refused.
static fdb_error_t raise_fence(FDBTransaction *tr, FdbFile *file) {
	uint8_t key[KEYMAX];
	uint64_t fence = 0;
	int got = 0;

	int klen = key_meta(key, file->name, "FENCE");
	fdb_error_t err = get_u64(tr, key, klen, &fence, &got);
	if (err) return err;

	fence = got ? fence + 1 : 1;
	set_u64(tr, key, klen, fence);
	file->fence = (int64_t)fence;
	return 0;
}

static fdb_error_t open_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	FdbFile *file = ((struct open_ctx *)ctx)->f;
	fdb_error_t err;

	// An attempt may run again, so it starts from a known state.
	file->head = 0;
	file->size = 0;
	file->has_shard = 0;
	file->shard_as_of = 0;

	if ((err = load_head(tr, file))) return err;
	if ((err = load_newest_shard(tr, file))) return err;
	// A reader follows the committed head, so it neither tidies a writer's unfinished
	// commit nor takes the fence a writer needs.
	if (((struct open_ctx *)ctx)->readonly) {
		file->sent_size = file->size;
		return 0;
	}
	drop_unfinished_commit(tr, file);
	if ((err = raise_fence(tr, file))) return err;

	file->sent_size = file->size;
	return 0;
}

// ── Read ──────────────────────────────────────────────────────────────────────

struct read_ctx {
	FdbFile *f;
	uint8_t *out;
	int amt;
	sqlite3_int64 off;
	int short_read;
};

static fdb_error_t read_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct read_ctx *r = ctx;
	FdbFile *f = r->f;

	// An attempt starts over, so clear what an earlier one wrote.
	memset(r->out, 0, (size_t)r->amt);
	r->short_read = 0;

	for (int done = 0; done < r->amt;) {
		sqlite3_int64 at = r->off + done;
		uint32_t pgno = (uint32_t)(at / PAGE);
		int within = (int)(at % PAGE);
		int want = r->amt - done;
		if (want > PAGE - within) want = PAGE - within;

		// A page SQLite wrote and the store has not taken yet is the truth.
		int slot;
		if (find_dirty(f, pgno, &slot)) {
			memcpy(r->out + done, f->dirty[slot]->bytes + within, (size_t)want);
			done += want;
			continue;
		}

		uint8_t page[PAGE];
		int len = 0, present = 0;
		fdb_error_t err = page_from_store(tr, f, pgno, page, &len, &present);
		if (err) return err;

		if (present) {
			int have = len - within;
			if (have > 0) memcpy(r->out + done, page + within, have < want ? (size_t)have : (size_t)want);
			if (have < want) r->short_read = 1;
		} else {
			r->short_read = 1;
		}
		done += want;
	}
	return 0;
}

// Overlay four big-endian bytes at `at` onto a read of [off, off + amt).
static void overlay_u32(uint8_t *buf, int amt, sqlite3_int64 off, sqlite3_int64 at, uint32_t v) {
	uint8_t be[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v};
	for (int i = 0; i < 4; i++) {
		sqlite3_int64 pos = at + i - off;
		if (pos >= 0 && pos < amt) buf[pos] = be[i];
	}
}

static int fdb_read(sqlite3_file *file, void *buf, int amt, sqlite3_int64 off) {
	FdbFile *f = (FdbFile *)file;
	struct read_ctx r = {f, buf, amt, off, 0};
	int rc = run_read_txn(read_body, &r, f);
	if (rc != SQLITE_OK) return rc;
	f->cache_droppable = 0; // SQLite now holds a page it may use without re-reading the header

	// A reader's change counter is the head. The header's own counter (byte 24) stops
	// moving once the writer holds EXCLUSIVE locking, and it is the counter SQLite compares
	// at the start of a transaction to decide whether its page cache is still good. Byte 92
	// is the counter the header's page count was valid for, and it is kept equal so SQLite
	// goes on trusting that count. See `check_view`.
	if (f->readonly && off < 96) {
		overlay_u32(buf, amt, off, 24, (uint32_t)f->head);
		overlay_u32(buf, amt, off, 92, (uint32_t)f->head);
	}
	// SQLite needs the short-read code so it can zero-fill and grow the file.
	return r.short_read ? SQLITE_IOERR_SHORT_READ : SQLITE_OK;
}

// ── Write ─────────────────────────────────────────────────────────────────────

// A write goes into memory. The store takes it when SQLite syncs, so the pages of one
// SQLite commit reach FoundationDB as one transaction.
//
// A write that covers part of a page has to keep the bytes already stored beside the new
// ones, so it reads that page first.
// Give back the buffered copy of a page, adding it when it is not there yet.
//
// `whole` says the caller is about to overwrite every byte of the page, so the page in
// the store does not have to be read first. A write that covers only part of a page has
// to keep the bytes already stored beside the new ones.
static DirtyPage *buffer_page(FdbFile *file, uint32_t pgno, int whole, int *rc) {
	int slot;
	*rc = SQLITE_OK;

	if (find_dirty(file, pgno, &slot)) return file->dirty[slot];

	if ((int64_t)(file->ndirty + 1) * file->pidx_row > FDB_TXN_LIMIT - HEAD_TXN_RESERVE) {
		// The index rows of this commit no longer fit one transaction, so the commit
		// could not be made atomic.
		*rc = SQLITE_IOERR_WRITE;
		return NULL;
	}

	// Read the stored page before the buffer holds it, so the read goes to the store.
	//
	// Rung 4: the read-ahead window may already hold this page from an earlier read
	// transaction. Under EXCLUSIVE mode + fence, nothing else has changed the store since,
	// and `ra_reset` fires on every event that could — a commit, a fold, a truncate — so
	// a window hit is the same bytes a fresh read would produce. `spec/PrewriteCoalesce.lean`
	// is `coalesced_prewrite_read_agrees`. Saves one whole FDB transaction per partial
	// write, which under YCSB workload F is every RMW.
	uint8_t stored[PAGE];
	memset(stored, 0, PAGE);
	if (!whole) {
		int hit_len = 0, hit_present = 0;
		if (!ra_hit(file, pgno, stored, &hit_len, &hit_present)) {
			struct read_ctx r = {file, stored, PAGE, (sqlite3_int64)pgno * PAGE, 0};
			int err = run_read_txn(read_body, &r, file);
			if (err != SQLITE_OK && err != SQLITE_IOERR_SHORT_READ) {
				*rc = err;
				return NULL;
			}
		}
	}

	DirtyPage *page = insert_dirty(file, pgno, slot);
	if (!page) {
		*rc = SQLITE_IOERR_NOMEM;
		return NULL;
	}
	if (!whole) memcpy(page->bytes, stored, PAGE);
	return page;
}

static int fdb_write(sqlite3_file *file, const void *buf, int amt, sqlite3_int64 off) {
	FdbFile *f = (FdbFile *)file;
	const uint8_t *in = buf;

	for (int done = 0; done < amt;) {
		sqlite3_int64 at = off + done;
		uint32_t pgno = (uint32_t)(at / PAGE);
		int within = (int)(at % PAGE);
		int want = amt - done;
		if (want > PAGE - within) want = PAGE - within;

		int rc = SQLITE_OK;
		DirtyPage *page = buffer_page(f, pgno, within == 0 && want == PAGE, &rc);
		if (!page) return rc;

		memcpy(page->bytes + within, in + done, (size_t)want);
		done += want;
	}

	if (off + amt > f->size) f->size = off + amt;
	return SQLITE_OK;
}

// ── Commit ────────────────────────────────────────────────────────────────────

struct flush_ctx {
	FdbFile *f;
	uint64_t txid;
	int lo, hi; // the pages this transaction carries
};

// Refuse the transaction unless this handle still owns the database.
//
// Why there is a fence at all. The locks above are no-ops, so two writers both believe
// they hold the write lock. Run that way, both reported success, `PRAGMA integrity_check`
// passed, and one writer's 300 rows were simply gone: silent loss against a database that
// checks out as healthy. The fence turned that into a refusal the caller can see. A store
// may refuse a write, and it may not accept a write and drop it.
//
// The single-writer invariant is not absolute, which is why this cannot be assumed away.
// `Weft.Actors` says Horde is CRDT-based and chooses availability, so during a partition
// each side may briefly run its own instance. The actor supervisor reaches the same
// answer in its depot client, whose `is_head_fence_mismatch` predicate is the fence
// this file writes.
//
// Every write transaction reads the fence, not only the commit. The first version checked
// it in the commit alone, so a stale writer could still compact. Compaction drops the
// shard version the owner is reading, the owner then read pages that were gone, and both
// writers failed against a database that was intact. A fence that covers one write path
// and not the others is not a fence. Reading the fence here also makes FoundationDB reject
// the transaction if the fence moves before it commits.
static fdb_error_t check_fence(FDBTransaction *tr, FdbFile *file, int *final) {
	uint8_t key[KEYMAX];
	uint64_t fence = 0;
	int got = 0;

	int klen = key_meta(key, file->name, "FENCE");
	fdb_error_t err = get_u64(tr, key, klen, &fence, &got);
	if (err) return err;

	// Refusing the write is the correct answer, so it must reach the caller instead of
	// being retried.
	if (!got || (int64_t)fence != file->fence) *final = SQLITE_READONLY;
	return 0;
}

// Refuse a read unless the store is still the one this handle's cache was built on.
//
// A handle caches pages: SQLite's page cache under EXCLUSIVE locking, the dirty buffer and
// the read-ahead window. A fence that moved means another writer has changed the store, so
// a page read now and a page cached earlier come from two databases, and a B-tree assembled
// from both is corrupt. SQLite trusts a page it is handed and follows a pointer off its end:
// Uro's upload after a lost fence was a SIGSEGV in sqlite3BtreeInsert. The fence covered
// every write transaction and no read transaction, and a fence that covers one path and not
// the others is not a fence. `spec/ViewCoherence.lean` enumerates every body and proves the
// file-scoped ones check.
//
// A writer's view is its fence, and a stale writer fails every time, because the database
// it was reading is not there any more.
//
// A reader holds no fence, so its view is the head and the shard version it loaded. A fold
// moves the shard and changes no page, so the view is reloaded and the read goes on. A
// commit moves the head, and the reader's cache is then wrong unless SQLite is about to
// drop it. SQLite drops it when the header's change counter differs from the one it holds,
// which it reads at the start of every transaction under NORMAL locking and never under
// EXCLUSIVE, and a writer under EXCLUSIVE locking stops bumping that counter after its
// first commit. So `fdb_read` reports the head as a reader's change counter, and the moved
// head is reloaded only when SQLite is between transactions (`cache_droppable`, set by
// `fdb_unlock`). Inside a transaction, or under EXCLUSIVE locking, the read is refused and
// stays refused: an I/O error does not empty SQLite's cache either, so the handle reopens.
//
// The check runs when a read transaction starts, and every read in that transaction is at
// the read version it saw, so a handle reads one store state per transaction: its own, or
// nothing. A cached transaction is replaced after four seconds, after a refused write, and
// for a handle under NORMAL locking at the end of each SQLite transaction (`fdb_unlock`).
//
// This is what keeps a SQLite transaction inside FoundationDB's semantics when it outlives
// one FoundationDB transaction, which a long read does at the four-second mark and a large
// commit does by staging. A SQLite transaction sees one store state, as a FoundationDB
// transaction does: each later read version it runs on is either the same state, because
// the fence or the head has not moved, or it is refused. Staged pages are the write side of
// the same rule, invisible until the head moves.
//
// What this cannot reach is a read SQLite answers from its own cache without calling the
// VFS. Under EXCLUSIVE locking that is by design, and it hands SQLite nothing from the
// store, so no page is mixed. A reader that wants to follow the head keeps NORMAL locking,
// where SQLite reads page 1 at every transaction and this check runs.
static fdb_error_t check_view(FDBTransaction *tr, FdbFile *f, int *final) {
	if (plant_stale_view()) return 0;
	if (!f->readonly) {
		fdb_error_t err = check_fence(tr, f, final);
		if (!err && *final) *final = SQLITE_IOERR_READ;
		return err;
	}

	uint8_t key[KEYMAX], from[KEYMAX], to[KEYMAX];
	uint64_t head = 0, as_of = 0;
	int got = 0, found = 0;

	int klen = key_meta(key, f->name, "HEAD");
	fdb_error_t err = get_u64(tr, key, klen, &head, &got);
	if (err) return err;
	if (!got) head = 0;

	int flen = key_prefix(from, f->name, "SHARDN");
	int tlen = key_shardn(to, f->name, head + 1);
	if ((err = edge_number(tr, from, flen, to, tlen, 1, &as_of, &found))) return err;

	if (head == f->head && found == f->has_shard && (!found || as_of == f->shard_as_of)) return 0;

	if (head != f->head && !f->cache_droppable) {
		*final = SQLITE_IOERR_READ;
		return 0;
	}
	if ((err = load_head(tr, f))) return err;
	if ((err = load_newest_shard(tr, f))) return err;
	f->sent_size = f->size;
	f->cache_droppable = 0;
	ra_reset(f);
	return 0;
}

// The pages of the commit, under a txid no read can reach yet.
static void put_delta_pages(FDBTransaction *tr, struct flush_ctx *c) {
	uint8_t key[KEYMAX], d[PAGE];
	for (int i = c->lo; i < c->hi; i++) {
		DirtyPage *p = c->f->dirty[i];
		int klen = key_delta(key, c->f->name, c->txid, p->pgno);
		BaseEntry *b = base_get(c->f, p->pgno);
		int n = b ? delta_encode(b->bytes, p->bytes, b->kind, b->id, d) : 0;
		p->as_delta = n > 0;
		if (n > 0) fdb_transaction_set(tr, key, klen, d, n);
		else fdb_transaction_set(tr, key, klen, p->bytes, PAGE);
	}
}

static fdb_error_t delta_body(FDBTransaction *tr, void *ctx, int *final) {
	struct flush_ctx *c = ctx;
	fdb_error_t err = check_fence(tr, c->f, final);
	if (err || *final) return err;
	put_delta_pages(tr, c);
	return 0;
}

// What makes a commit visible: PIDX points at the new txid, the size moves, and the head
// advances. All of it lands together or none of it does.
static fdb_error_t put_head(FDBTransaction *tr, struct flush_ctx *c) {
	FdbFile *f = c->f;
	uint8_t key[KEYMAX], to[KEYMAX];
	int klen;

	for (int i = 0; i < f->ndirty; i++) {
		klen = key_pidx(key, f->name, f->dirty[i]->pgno);
		set_u64(tr, key, klen, c->txid);
	}

	// A truncate drops the index rows of the pages it removed. The pages themselves
	// stay where they are, unreachable, and the next compaction does not fold them.
	if (f->trunc_pages >= 0) {
		klen = key_pidx(key, f->name, (uint32_t)f->trunc_pages);
		int plen = key_prefix(to, f->name, "PIDX");
		int tlen = key_after(to, to, plen);
		fdb_transaction_clear_range(tr, key, klen, to, tlen);
	}

	klen = key_meta(key, f->name, "SIZE");
	set_u64(tr, key, klen, (uint64_t)f->size);

	// The absolute value, not a read and a sum. A single writer knows what the log holds,
	// so reading it back would cost a round trip to learn a number it already has.
	klen = key_meta(key, f->name, "LOGN");
	set_u64(tr, key, klen, f->log_pages + (uint64_t)f->ndirty);

	klen = key_meta(key, f->name, "HEAD");
	set_u64(tr, key, klen, c->txid);
	return 0;
}

static fdb_error_t head_body(FDBTransaction *tr, void *ctx, int *final) {
	struct flush_ctx *c = ctx;
	fdb_error_t err = check_fence(tr, c->f, final);
	if (err || *final) return err;
	return put_head(tr, c);
}

// The whole commit in one transaction: the pages and the head together.
//
// A commit that fits one transaction needs no staging. The pages are never visible
// before the head moves, because they arrive with it.
static fdb_error_t commit_body(FDBTransaction *tr, void *ctx, int *final) {
	struct flush_ctx *c = ctx;
	fdb_error_t err = check_fence(tr, c->f, final);
	if (err || *final) return err;
	put_delta_pages(tr, c);
	return put_head(tr, c);
}

// The planted torn commit: one page, its index row and the head, in one transaction.
static fdb_error_t torn_body(FDBTransaction *tr, void *ctx, int *final) {
	struct flush_ctx *c = ctx;
	FdbFile *f = c->f;
	uint8_t key[KEYMAX];
	fdb_error_t err = check_fence(tr, f, final);
	if (err || *final) return err;
	put_delta_pages(tr, c);
	int klen = key_pidx(key, f->name, f->dirty[c->lo]->pgno);
	set_u64(tr, key, klen, c->txid);
	klen = key_meta(key, f->name, "SIZE");
	set_u64(tr, key, klen, (uint64_t)f->size);
	klen = key_meta(key, f->name, "LOGN");
	set_u64(tr, key, klen, f->log_pages + (uint64_t)c->hi);
	klen = key_meta(key, f->name, "HEAD");
	set_u64(tr, key, klen, c->txid);
	return 0;
}

// The planted slow commit: a fence read in a transaction of its own.
static fdb_error_t slow_body(FDBTransaction *tr, void *ctx, int *final) {
	struct flush_ctx *c = ctx;
	return check_fence(tr, c->f, final);
}

static int compact(FdbFile *f);
static int should_compact(FdbFile *f);

// Send everything SQLite wrote as one commit.
//
// A commit is one transaction whenever it fits one, which is the common case and the
// cheap one. It costs a single round trip, and there is no window where the pages exist
// and the head does not.
//
// A commit too large for one transaction stages instead. The pages go first, under a
// txid no read can reach, and one more transaction then moves the head. This is the
// shape CockroachDB calls a parallel commit: the writes are staged, and the commit is
// the single record that makes them real. The staged pages are safe to leave behind,
// because `drop_unfinished_commit` clears any txid above the head at the next open.
static int txn_stage_here(FdbFile *f);

static int flush(FdbFile *f) {
	if (f->ndirty == 0 && f->trunc_pages < 0 && f->size == f->sent_size) return SQLITE_OK;

	// A file that joined a group does not commit when SQLite says so. It stages, and the
	// group's record is what commits it later.
	//
	// This has to happen here rather than in a call after the fact. SQLite ends a
	// statement by writing its pages and calling xSync, and this is xSync: by the time a
	// caller could ask to stage, an ordinary flush would already have moved the head and
	// the write would be visible on its own. That is the whole failure this catches.
	if (f->group_txnid) return txn_stage_here(f);

	struct flush_ctx c = {f, f->head + 1, 0, f->ndirty};
	int rc;

	if (plant_slow_commit()) {
		rc = run_txn(slow_body, &c, 0, SQLITE_IOERR_WRITE);
		if (rc != SQLITE_OK) return rc;
	}

	if (plant_torn_commit() && f->trunc_pages < 0 && f->ndirty > 0) {
		rc = SQLITE_OK;
		for (int lo = 0; lo < f->ndirty && rc == SQLITE_OK; lo++) {
			c.lo = lo;
			c.hi = lo + 1;
			rc = run_txn(torn_body, &c, 1, SQLITE_IOERR_WRITE);
		}
	} else if (f->ndirty <= ONE_TXN_PAGES) {
		rc = run_txn(commit_body, &c, 1, SQLITE_IOERR_WRITE);
	} else {
		rc = SQLITE_OK;
		for (int lo = 0; lo < f->ndirty && rc == SQLITE_OK; lo += STAGE_TXN_PAGES) {
			c.lo = lo;
			c.hi = lo + STAGE_TXN_PAGES;
			if (c.hi > f->ndirty) c.hi = f->ndirty;
			rc = run_txn(delta_body, &c, 1, SQLITE_IOERR_WRITE);
		}
		if (rc == SQLITE_OK) rc = run_txn(head_body, &c, 1, SQLITE_IOERR_WRITE);
	}
	if (rc != SQLITE_OK) {
		// A refused write told this handle the fence moved. Its cached read transaction is a
		// snapshot of the store it used to own, so the next read starts a fresh one and
		// `check_view` refuses it, rather than reading the old snapshot until the four
		// seconds run out.
		if (rc == SQLITE_READONLY && f->ro_tr) {
			fdb_transaction_destroy(f->ro_tr);
			f->ro_tr = NULL;
		}
		return rc;
	}

	f->head = c.txid;
	f->sent_size = f->size;
	f->log_pages += (uint64_t)f->ndirty;
	// A page sent whole is now its own base. A page sent as a delta keeps the base it was
	// taken against, which is still a full row, so the next delta stays one link deep.
	for (int i = 0; i < f->ndirty; i++) {
		DirtyPage *p = f->dirty[i];
		if (!p->as_delta || plant_delta_chain()) {
			base_put(f, p->pgno, BASE_TXID, c.txid, p->bytes);
		} else if (plant_delta_stale()) {
			BaseEntry *b = base_get(f, p->pgno);
			if (b) memcpy(b->bytes, p->bytes, PAGE);
		}
	}
	clear_dirty(f);
	ra_reset(f); // those pages are not what the window holds any more
	if (f->ro_tr) {
		fdb_transaction_destroy(f->ro_tr);
		f->ro_tr = NULL;
	}

	// Note that a fold is owed; do not do it here. Folding on the commit that happens to
	// trip the ratio charges one writer for work every writer caused, and the bill grows
	// with the database: measured at 1.01 ms for an ordinary commit and 85 ms for the one
	// that folded, on a run of 900. `weft_compact_due` is where it gets paid.
	if (should_compact(f)) f->compact_due = 1;
	return SQLITE_OK;
}

// ── Compaction ────────────────────────────────────────────────────────────────
//
// Compaction folds the log into a new shard version. It adds a version and never
// overwrites one, and it clears a PIDX row only when that row points at a folded txid.
// `Store.lean` proves that these two rules preserve every read.
//
// The trigger is a ratio, not a number: fold when the log is as large as the base. A
// ratio has no units to tune, and it moves with the load. A quiet actor never compacts.

// Compact when the log is as large as the base. `Store.lean` calls this shouldCompact.
//
// Both sizes live in the handle, so the decision costs nothing. It used to cost two round
// trips after every commit, which was more than the commit itself.
static int should_compact(FdbFile *f) {
	return f->log_pages > 0 && f->base_pages <= f->log_pages;
}

// Read every row of [from, to), following `more` until the range is done, and hand each
// page-keyed row to `each`. The page number is the key's last four bytes.
typedef void (*page_row)(void *ctx, uint32_t pgno, const uint8_t *val, int vlen);

static fdb_error_t range_pages(FDBTransaction *tr, const uint8_t *from, int flen,
                               const uint8_t *to, int tlen, page_row each, void *ctx) {
	uint8_t begin[KEYMAX];
	int blen = flen;
	int after = 0;
	memcpy(begin, from, (size_t)flen);
	for (;;) {
		FDBFuture *fu = fdb_transaction_get_range(tr, begin, blen, after, 1, to, tlen, 0, 1, 0, 0,
		                                          FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);
		fdb_error_t err = await(fu);
		const FDBKeyValue *kv = NULL;
		int n = 0;
		fdb_bool_t more = 0;
		if (!err) err = fdb_future_get_keyvalue_array(fu, &kv, &n, &more);
		if (!err) {
			for (int i = 0; i < n; i++) {
				if (kv[i].key_length < 4) continue;
				const uint8_t *p = kv[i].key + kv[i].key_length - 4;
				uint32_t pgno = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
				                | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
				each(ctx, pgno, kv[i].value, kv[i].value_length);
			}
			if (more && n > 0 && kv[n - 1].key_length <= KEYMAX) {
				memcpy(begin, kv[n - 1].key, (size_t)kv[n - 1].key_length);
				blen = kv[n - 1].key_length;
				after = 1;
			} else {
				more = 0;
			}
		}
		fdb_future_destroy(fu);
		if (err) return err;
		if (!more) return 0;
	}
}

struct fold_ctx {
	FdbFile *f;
	uint64_t as_of;
	uint32_t lo, hi;  // the window of pages this pass folds
	uint8_t *pages;   // hi - lo pages
	uint8_t *present; // one flag for each page in the window
	uint64_t *owner;  // the txid whose DELTA holds each page, 0 for none
};

static void fold_owner(void *ctx, uint32_t pgno, const uint8_t *val, int vlen) {
	struct fold_ctx *c = ctx;
	if (pgno >= c->lo && pgno < c->hi && vlen == 8) c->owner[pgno - c->lo] = get_be64(val);
}

static void fold_shard(void *ctx, uint32_t pgno, const uint8_t *val, int vlen) {
	struct fold_ctx *c = ctx;
	if (pgno < c->lo || pgno >= c->hi || c->owner[pgno - c->lo]) return;
	memcpy(c->pages + (size_t)(pgno - c->lo) * PAGE, val, (size_t)(vlen > PAGE ? PAGE : vlen));
	c->present[pgno - c->lo] = 1;
}

// Read a window of pages with the answers `page_from_store` gives, one page at a time,
// but pipelined: the index and the shard as two range reads, then every logged page's
// read in flight at once. Folding a log page by page cost two round trips a page, which
// was ten seconds at close for a 25 000 row table.
static fdb_error_t fold_read_body(FDBTransaction *tr, void *ctx, int *final) {
	struct fold_ctx *c = ctx;
	FdbFile *f = c->f;
	const uint32_t n = c->hi - c->lo;
	memset(c->present, 0, n);
	memset(c->owner, 0, (size_t)n * sizeof(uint64_t));
	memset(c->pages, 0, (size_t)n * PAGE);

	fdb_error_t err = check_fence(tr, f, final);
	if (err || *final) return err;

	uint8_t from[KEYMAX], to[KEYMAX];
	int flen = key_pidx(from, f->name, c->lo);
	int tlen = key_pidx(to, f->name, c->hi);
	if ((err = range_pages(tr, from, flen, to, tlen, fold_owner, c))) return err;

	if (f->has_shard) {
		flen = key_shard(from, f->name, f->shard_as_of, c->lo);
		tlen = key_shard(to, f->name, f->shard_as_of, c->hi);
		if ((err = range_pages(tr, from, flen, to, tlen, fold_shard, c))) return err;
	}

	FDBFuture **fu = calloc(n, sizeof(FDBFuture *));
	if (!fu) {
		*final = SQLITE_IOERR_NOMEM;
		return 0;
	}
	for (uint32_t i = 0; i < n; i++) {
		if (!c->owner[i]) continue;
		uint8_t key[KEYMAX];
		int klen = key_delta(key, f->name, c->owner[i], c->lo + i);
		fu[i] = fdb_transaction_get(tr, key, klen, 0);
	}
	// A logged page that is a delta needs its base, which is a second wave in flight.
	uint8_t **d = calloc(n, sizeof(uint8_t *));
	int *dlen = calloc(n, sizeof(int));
	if (!d || !dlen) err = 2000;
	for (uint32_t i = 0; i < n; i++) {
		if (!fu[i]) continue;
		fdb_error_t e = await(fu[i]);
		fdb_bool_t have = 0;
		const uint8_t *val;
		int vlen;
		if (!e) e = fdb_future_get_value(fu[i], &have, &val, &vlen);
		if (!e && have && d && is_delta(val, vlen)) {
			d[i] = malloc((size_t)vlen);
			if (d[i]) {
				memcpy(d[i], val, (size_t)vlen);
				dlen[i] = vlen;
			} else if (!err) {
				err = 2000;
			}
		} else if (!e && have) {
			memcpy(c->pages + (size_t)i * PAGE, val, (size_t)(vlen > PAGE ? PAGE : vlen));
			c->present[i] = 1;
		}
		if (e && !err) err = e;
		fdb_future_destroy(fu[i]);
		fu[i] = NULL;
	}
	for (uint32_t i = 0; i < n && !err; i++) {
		if (!d || !d[i]) continue;
		uint8_t key[KEYMAX];
		int klen = base_key(key, f, d[i][1], delta_base_id(d[i]), c->lo + i);
		if (klen < 0) err = 2000;
		else fu[i] = fdb_transaction_get(tr, key, klen, 0);
	}
	for (uint32_t i = 0; i < n; i++) {
		if (!fu[i]) continue;
		fdb_error_t e = await(fu[i]);
		fdb_bool_t have = 0;
		const uint8_t *val;
		int vlen;
		if (!e) e = fdb_future_get_value(fu[i], &have, &val, &vlen);
		uint8_t *slot = c->pages + (size_t)i * PAGE;
		if (!e && (!have || vlen != PAGE)) e = 2000; // a base is a full row
		if (!e) {
			memcpy(slot, val, PAGE);
			if (delta_apply(d[i], dlen[i], slot) != 0) e = 2000;
			else c->present[i] = 1;
		}
		if (e && !err) err = e;
		fdb_future_destroy(fu[i]);
	}
	for (uint32_t i = 0; d && i < n; i++) free(d[i]);
	free(d);
	free(dlen);
	free(fu);
	return err;
}

static fdb_error_t fold_write_body(FDBTransaction *tr, void *ctx, int *final) {
	struct fold_ctx *c = ctx;
	uint8_t key[KEYMAX];

	fdb_error_t err = check_fence(tr, c->f, final);
	if (err || *final) return err;

	for (uint32_t pgno = c->lo; pgno < c->hi; pgno++) {
		if (!c->present[pgno - c->lo]) continue;
		int klen = key_shard(key, c->f->name, c->as_of, pgno);
		fdb_transaction_set(tr, key, klen, c->pages + (size_t)(pgno - c->lo) * PAGE, PAGE);
	}
	return 0;
}

struct finish_ctx {
	FdbFile *f;
	uint64_t as_of;
	uint32_t kept;
};

// Make the new shard version the one a read uses, and drop what it replaced. One
// transaction, so a reader sees the old version or the new one.
static fdb_error_t finish_body(FDBTransaction *tr, void *ctx, int *final) {
	struct finish_ctx *c = ctx;
	FdbFile *f = c->f;
	uint8_t key[KEYMAX], from[KEYMAX], to[KEYMAX];

	fdb_error_t err = check_fence(tr, f, final);
	if (err || *final) return err;

	// The marker that makes the version usable. Until this lands, a read keeps the old
	// base and the log.
	int klen = key_shardn(key, f->name, c->as_of);
	set_u64(tr, key, klen, c->kept);

	// Every PIDX row points at a folded txid, because the fold covered the head. So the
	// whole index goes, and every page now comes from the new shard.
	clear_prefix(tr, f->name, "PIDX");
	clear_prefix(tr, f->name, "DELTA");

	// Every shard version below the new one goes, because a read is served at the head
	// and nothing holds an older version. `Store.lean` calls this evict, with the oldest
	// pin at the head. A read below the head needs a pin to hold its version, and nothing
	// writes one, so there is no pin to read here. Adding that read before a reader needs
	// it would cost a round trip on every compaction to find nothing.
	uint64_t keep_from = c->as_of;
	int flen = key_shard_version(from, f->name, 0);
	int tlen = key_shard_version(to, f->name, keep_from);
	fdb_transaction_clear_range(tr, from, flen, to, tlen);
	flen = key_shardn(from, f->name, 0);
	tlen = key_shardn(to, f->name, keep_from);
	fdb_transaction_clear_range(tr, from, flen, to, tlen);

	klen = key_meta(key, f->name, "LOGN");
	set_u64(tr, key, klen, 0);
	return 0;
}

// A fold sends a window's shard rows and reads the next window while they commit, with
// up to FOLD_DEPTH windows' commits in flight. One window at a time spent the read and
// the commit in turn: of 2.5 s at close for a 35 MB database, 1.4 s was waiting on a
// commit with nothing else to do. Smaller windows commit faster, since FoundationDB
// resolves a 10 MB transaction slowly; the depth keeps memory near the old single window.
#define FOLD_WINDOW_PAGES (STAGE_TXN_PAGES / 4)
#define FOLD_DEPTH 4

struct fold_slot {
	struct fold_ctx c;
	FDBTransaction *tr;
	FDBFuture *fu; // the window's commit, while it is in flight
	int sent;      // the window has rows to settle
};

// Wait for a window's commit. A commit that failed goes again through `run_txn`, which
// knows which errors may be retried and checks the fence; the rows are the same rows
// under a shard version no read uses yet, so writing them twice is harmless.
static int fold_settle(struct fold_slot *s, uint32_t *kept) {
	if (!s->sent) return SQLITE_OK;
	int again = 1;
	if (s->fu) {
		again = await(s->fu) != 0;
		fdb_future_destroy(s->fu);
		s->fu = NULL;
	}
	if (s->tr) {
		fdb_transaction_destroy(s->tr);
		s->tr = NULL;
	}
	s->sent = 0;
	if (again) {
		int rc = run_txn(fold_write_body, &s->c, 1, SQLITE_IOERR_WRITE);
		if (rc != SQLITE_OK) return rc;
	}
	for (uint32_t i = 0; i < s->c.hi - s->c.lo; i++)
		if (s->c.present[i]) (*kept)++;
	return SQLITE_OK;
}

// Start a window's commit and return without waiting for it. Anything that stops the
// commit from starting leaves the window to `fold_settle`, which sends it the slow way.
static int fold_send(struct fold_slot *s) {
	s->sent = 1;
	if (fdb_database_create_transaction(g_db, &s->tr)) {
		s->tr = NULL;
		return SQLITE_OK;
	}
	int final = 0;
	fdb_error_t err = fold_write_body(s->tr, &s->c, &final);
	if (final) return final;
	if (err) return SQLITE_OK;
	crash_point();
	s->fu = fdb_transaction_commit(s->tr);
	return SQLITE_OK;
}

static int compact(FdbFile *f) {
	uint64_t as_of = f->head;
	uint32_t npages = (uint32_t)((f->size + PAGE - 1) / PAGE);
	if (npages == 0) return SQLITE_OK;

	uint32_t window = FOLD_WINDOW_PAGES;
	if (window > npages) window = npages;

	struct fold_slot slot[FOLD_DEPTH];
	memset(slot, 0, sizeof slot);
	int rc = SQLITE_OK;
	for (int i = 0; i < FOLD_DEPTH; i++) {
		struct fold_ctx *c = &slot[i].c;
		c->f = f;
		c->as_of = as_of;
		c->pages = malloc((size_t)window * PAGE);
		c->present = malloc(window);
		c->owner = malloc((size_t)window * sizeof(uint64_t));
		if (!c->pages || !c->present || !c->owner) rc = SQLITE_IOERR_NOMEM;
	}

	uint32_t kept = 0;
	uint32_t k = 0;
	for (uint32_t lo = 0; lo < npages && rc == SQLITE_OK; lo += window, k++) {
		struct fold_slot *s = &slot[k % FOLD_DEPTH];
		if ((rc = fold_settle(s, &kept)) != SQLITE_OK) break;
		s->c.lo = lo;
		s->c.hi = lo + window;
		if (s->c.hi > npages) s->c.hi = npages;
		if ((rc = run_txn(fold_read_body, &s->c, 0, SQLITE_IOERR_READ)) != SQLITE_OK) break;
		rc = fold_send(s);
	}
	// Every window settles, even after a failure, so no commit is left in flight with
	// its buffer freed under it.
	for (int i = 0; i < FOLD_DEPTH; i++) {
		int r = SQLITE_OK;
		if (rc == SQLITE_OK) {
			r = fold_settle(&slot[i], &kept);
		} else if (slot[i].fu) {
			await(slot[i].fu);
			fdb_future_destroy(slot[i].fu);
		}
		if (rc != SQLITE_OK && slot[i].tr) fdb_transaction_destroy(slot[i].tr);
		if (rc == SQLITE_OK) rc = r;
		free(slot[i].c.pages);
		free(slot[i].c.present);
		free(slot[i].c.owner);
	}
	if (rc != SQLITE_OK) return rc;

	struct finish_ctx fin = {f, as_of, kept};
	if ((rc = run_txn(finish_body, &fin, 1, SQLITE_IOERR_WRITE)) != SQLITE_OK) return rc;

	f->has_shard = 1;
	f->shard_as_of = as_of;
	f->base_pages = kept;
	f->log_pages = 0;
	// The fold deleted every log row and the old shard, which is every base this handle
	// knew. The pages read next become the new bases.
	if (!plant_delta_fold()) base_clear(f);
	ra_reset(f); // the window points at a shard version that is no longer the one read
	return SQLITE_OK;
}

// ── A commit across several databases ─────────────────────────────────────────
//
// `flush` commits one file, because that is what SQLite gives it: one `xSync` for one
// database. So two databases cannot be committed together by anything above, and a grant
// that takes an item from one and gives it to another is two transactions with a gap.
//
// This is the parallel commit protocol, modelled in ParallelCommits.tla. The names below
// are that spec's, because a protocol implemented under different words is a protocol
// nobody can check against its model:
//
//   intent write          the pages of one participant, staged under a txid whose head has
//                         not moved. `flush` already writes exactly this shape for a
//                         commit too large for one transaction.
//   transaction record    `weft/txn/<txnid>`, holding the status and the participants.
//   implicitly committed  the record is staging, and every participant's intent is there.
//                         This is the commit point, and it costs one round.
//   explicitly committed  the record says committed, which recovery or the committer
//                         writes afterwards. It changes no read.
//   resolve intent        move that participant's head, which is what makes its staged
//                         pages reachable.
//   prevent               raise that database's fence, so the staging writer can never
//                         land. The fence is this layout's timestamp cache: it is the one
//                         thing that stops a write that has not happened yet.
//
// The order is what makes it safe. Every participant stages first, and no head has moved,
// so nothing is visible. One transaction then writes the record. That single write is the
// commit: after it lands the group is implicitly committed whether or not this process
// survives, because every intent is already durable and any other process can see both
// facts. Moving the heads afterwards changes nothing about whether it committed.
//
// A crash before the record leaves intents nobody can reach, and recovery aborts them. A
// crash after the record leaves a group that is committed but not resolved, and recovery
// finishes it. There is no third case, which is the property `prove_parallel_commit`
// looks for.
//
// One limit, and it is enforced rather than assumed: a participant must stage in one
// FoundationDB transaction. That makes an intent all or nothing, so recovery can decide a
// participant by reading one key instead of counting pages. A participant with more pages
// than one transaction carries is refused at stage time.

#define TXN_MAX_PARTS 16

// The group being built. One at a time for the process, under a lock, because a group
// spans databases that different threads own and the members must agree on the txid. Two
// groups at once would need a map from txid to members, and nothing needs that yet.
static pthread_mutex_t g_group_lock = PTHREAD_MUTEX_INITIALIZER;
static struct {
	uint64_t txnid;
	int nparts;
	FdbFile *files[TXN_MAX_PARTS];
	uint64_t txids[TXN_MAX_PARTS];
	int open;
} g_group;

struct seq_ctx {
	uint64_t txnid;
};

// The next group txid, from a counter in the store. Two stores must not choose the same
// one, so it is a read and a write in one transaction rather than a clock or a guess.
static fdb_error_t txn_seq_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct seq_ctx *c = ctx;
	uint8_t key[KEYMAX];
	uint64_t seq = 0;
	int got = 0;

	int klen = snprintf((char *)key, KEYMAX, "weft/txn/NEXT");
	fdb_error_t err = get_u64(tr, key, klen, &seq, &got);
	if (err) return err;

	c->txnid = got ? seq + 1 : 1;
	set_u64(tr, key, klen, c->txnid);
	return 0;
}

int weft_txn_begin(uint64_t *txnid) {
	pthread_mutex_lock(&g_group_lock);

	struct seq_ctx c = {0};
	int rc = run_txn(txn_seq_body, &c, 1, SQLITE_IOERR_WRITE);
	if (rc != SQLITE_OK) {
		pthread_mutex_unlock(&g_group_lock);
		return rc;
	}

	g_group.txnid = c.txnid;
	g_group.nparts = 0;
	g_group.open = 1;
	if (txnid) *txnid = c.txnid;
	return SQLITE_OK;
}

// True when this file belongs to this VFS. Defined below, beside the method table it
// compares against, so the table stays in one place.
static int is_fdb_file(const sqlite3_file *file);

// The FdbFile behind an open connection. SQLite hands back the `sqlite3_file` it made, and
// for this VFS that is an FdbFile, so no lookup table is needed.
//
// A connection on a different VFS reaches here if a caller passes one, and the check is
// what stops this treating some other VFS's file as an FdbFile and reading nonsense.
static FdbFile *file_of(sqlite3 *db) {
	sqlite3_file *file = NULL;
	if (sqlite3_file_control(db, "main", SQLITE_FCNTL_FILE_POINTER, &file) != SQLITE_OK) {
		return NULL;
	}
	return is_fdb_file(file) ? (FdbFile *)file : NULL;
}

// Join a database to the group, before it is written to.
//
// From here until the group commits, this file's syncs stage instead of committing. That
// ordering is the point: SQLite ends a statement by writing pages and calling xSync, so a
// caller cannot stage after the fact — the write would already be visible on its own.
int weft_txn_join(sqlite3 *db, uint64_t txnid) {
	FdbFile *f = file_of(db);
	if (!f || !g_group.open || g_group.txnid != txnid) return SQLITE_MISUSE;
	if (g_group.nparts >= TXN_MAX_PARTS) return SQLITE_FULL;

	// A participant that writes nothing is still a participant, so a reader of the record
	// sees the whole group. Its staged txid stays its head until a sync says otherwise.
	for (int i = 0; i < g_group.nparts; i++) {
		if (g_group.files[i] == f) return SQLITE_OK;
	}
	g_group.files[g_group.nparts] = f;
	g_group.txids[g_group.nparts] = f->head;
	g_group.nparts++;
	f->group_txnid = txnid;
	return SQLITE_OK;
}

// Write this participant's intent: its pages, under a txid whose head does not move.
//
// This is `flush`'s staging half with the head left alone on purpose. Nothing can read
// these pages, because no PIDX row points at them and the head is unchanged. The dirty
// buffer is kept, so this process still sees its own writes and so resolving has the page
// numbers it needs to build the index.
static int txn_stage_here(FdbFile *f) {
	// An intent has to be all or nothing, so recovery can decide a participant by reading
	// one key instead of counting pages.
	if (f->ndirty > ONE_TXN_PAGES) return SQLITE_TOOBIG;

	struct flush_ctx c = {f, f->head + 1, 0, f->ndirty};
	if (f->ndirty > 0) {
		int rc = run_txn(delta_body, &c, 1, SQLITE_IOERR_WRITE);
		if (rc != SQLITE_OK) return rc;
	}

	for (int i = 0; i < g_group.nparts; i++) {
		if (g_group.files[i] == f) {
			g_group.txids[i] = f->ndirty > 0 ? c.txid : f->head;
			return SQLITE_OK;
		}
	}
	// A sync on a file whose group flag is set but which is not in the group is a bug in
	// the caller's bookkeeping, not something to commit around.
	return SQLITE_MISUSE;
}

struct record_ctx {
	uint64_t txnid;
};

// The commit. One transaction writes the record as staging, naming every participant and
// the txid it staged under.
//
// After this lands the group is implicitly committed, and a caller may be told so. Every
// intent is already durable, and any process can now read the record and reach them, so
// the outcome no longer depends on this process being alive.
static fdb_error_t record_body(FDBTransaction *tr, void *ctx, int *final) {
	struct record_ctx *c = ctx;
	uint8_t key[KEYMAX];

	// The fence of every participant is read inside this transaction, so a participant
	// that lost ownership while staging cannot be carried into the group: FoundationDB
	// rejects the commit if any of those fences moved.
	for (int i = 0; i < g_group.nparts; i++) {
		fdb_error_t err = check_fence(tr, g_group.files[i], final);
		if (err || *final) return err;
	}

	// A participant row is the txid it staged under and the file size that commit ends
	// with. Recovery has no open handle, so everything it needs to move that head has to
	// be in the record: the page numbers it can read back from the staged DELTA rows, but
	// the size is nowhere else.
	for (int i = 0; i < g_group.nparts; i++) {
		uint8_t val[16];
		put_be64(val, g_group.txids[i]);
		put_be64(val + 8, (uint64_t)g_group.files[i]->size);
		int klen = key_txn_part(key, c->txnid, g_group.files[i]->name);
		fdb_transaction_set(tr, key, klen, val, 16);
	}

	uint8_t status = TXN_STAGING;
	int klen = key_txn_status(key, c->txnid);
	fdb_transaction_set(tr, key, klen, &status, 1);
	return 0;
}

static int txn_resolve_group(uint64_t txnid, int drop_record);

// Give up a group that was begun and not committed. No record was ever written, so there
// is nothing to decide: the staged pages are unreachable and the next recovery sweep will
// not even see them, because a group with no record is not a group.
//
// This exists because `weft_txn_begin` takes the lock and only a commit gives it back. A
// caller whose stage fails has to be able to let go, or the next group waits forever.
int weft_txn_abort(uint64_t txnid) {
	if (!g_group.open || g_group.txnid != txnid) return SQLITE_MISUSE;
	for (int i = 0; i < g_group.nparts; i++) g_group.files[i]->group_txnid = 0;
	g_group.open = 0;
	g_group.nparts = 0;
	pthread_mutex_unlock(&g_group_lock);
	return SQLITE_OK;
}

int weft_txn_commit(uint64_t txnid) {
	if (!g_group.open || g_group.txnid != txnid) return SQLITE_MISUSE;

	struct record_ctx c = {txnid};
	int rc = run_txn(record_body, &c, 1, SQLITE_IOERR_WRITE);
	if (rc != SQLITE_OK) {
		for (int i = 0; i < g_group.nparts; i++) g_group.files[i]->group_txnid = 0;
		g_group.open = 0;
		pthread_mutex_unlock(&g_group_lock);
		return rc;
	}

	// Implicitly committed from here. Resolving moves the heads, and a failure to resolve
	// is not a failure to commit: recovery finishes it.
	crash_point();
	(void)txn_resolve_group(txnid, 1);

	for (int i = 0; i < g_group.nparts; i++) g_group.files[i]->group_txnid = 0;
	g_group.open = 0;
	pthread_mutex_unlock(&g_group_lock);
	return SQLITE_OK;
}

// Move one participant's head, which is what makes its staged pages reachable. This is
// the resolve-intent step, and it may run more than once and from more than one process,
// so it must be safe to repeat.
static int txn_resolve_part(FdbFile *f, uint64_t staged) {
	if (staged <= f->head) return SQLITE_OK; // already resolved by somebody

	struct flush_ctx c = {f, staged, 0, f->ndirty};
	int rc = run_txn(head_body, &c, 1, SQLITE_IOERR_WRITE);
	if (rc != SQLITE_OK) return rc;

	f->head = staged;
	f->sent_size = f->size;
	f->log_pages += (uint64_t)f->ndirty;
	clear_dirty(f);
	ra_reset(f);
	if (f->ro_tr) {
		fdb_transaction_destroy(f->ro_tr);
		f->ro_tr = NULL;
	}
	return SQLITE_OK;
}

struct status_ctx {
	uint64_t txnid;
	uint8_t status;
};

static fdb_error_t set_status_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct status_ctx *c = ctx;
	uint8_t key[KEYMAX];
	int klen = key_txn_status(key, c->txnid);
	fdb_transaction_set(tr, key, klen, &c->status, 1);
	return 0;
}

static fdb_error_t drop_record_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct status_ctx *c = ctx;
	uint8_t from[KEYMAX], to[KEYMAX];
	int flen = key_txn_prefix(from, c->txnid);
	int tlen = key_after(to, from, flen);
	fdb_transaction_clear_range(tr, from, flen, to, tlen);
	return 0;
}

// Finish a group this process staged: move every head, then say so explicitly.
static int txn_resolve_group(uint64_t txnid, int drop_record) {
	for (int i = 0; i < g_group.nparts; i++) {
		int rc = txn_resolve_part(g_group.files[i], g_group.txids[i]);
		if (rc != SQLITE_OK) return rc; // recovery will finish what this did not
	}

	struct status_ctx c = {txnid, TXN_COMMITTED};
	int rc = run_txn(set_status_body, &c, 1, SQLITE_IOERR_WRITE);
	if (rc != SQLITE_OK) return rc;

	// The record has done its work once every head has moved. Keeping it would make the
	// sweep in recovery grow without bound.
	return drop_record ? run_txn(drop_record_body, &c, 1, SQLITE_IOERR_WRITE) : SQLITE_OK;
}

// Pay the fold that a commit noticed was owed.
//
// A caller runs this when it is between pieces of work rather than inside one: a shard
// thread with no request waiting, a job about to idle. Nothing here is hidden, and no
// thread is started to do it, because a fold that happens on its own schedule is a fold
// nobody can account for.
//
// Doing nothing is a valid outcome and the common one. The decision costs no round trip,
// since both sizes live in the handle.
int weft_compact_due(sqlite3 *db) {
	FdbFile *f = file_of(db);
	if (!f) return SQLITE_MISUSE;
	if (!f->compact_due) return SQLITE_OK;
	// A file in a group must not fold: compaction drops the shard version a reader is on,
	// and it would do it while the group's own writes are staged and unresolved.
	if (f->group_txnid) return SQLITE_OK;

	f->compact_due = 0;
	return compact(f);
}

// ── Recovery, which is the preventer process ──────────────────────────────────
//
// Anybody may find a staging record, and whoever does must decide it. The committer might
// be dead, so leaving it alone is not an option: its intents are pages nothing will ever
// reach and nothing will ever free.
//
// The decision is the one in the spec. Query every intent. If all of them are there the
// group is implicitly committed, and the only honest thing to do is finish it, whatever
// this process wanted. If any is missing it can never become committed, so prevent it —
// raise the fence on each participant so a late committer is refused — and abort.
//
// Both outcomes are idempotent, so two processes recovering the same record at once reach
// the same end.

// One participant, read back from the record.
typedef struct {
	char name[MAX_NAME];
	uint64_t txid;
	uint64_t size;
} TxnPart;

#define TXN_MAX_RECOVER 64

struct recover_ctx {
	uint64_t txnid;
	TxnPart parts[TXN_MAX_PARTS];
	int nparts;
	int all_present; // every intent found, so implicitly committed
};

// Read the participants of one record.
static fdb_error_t read_parts_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct recover_ctx *c = ctx;
	uint8_t from[KEYMAX], to[KEYMAX];

	c->nparts = 0;
	int flen = key_txn_part_prefix(from, c->txnid);
	int tlen = key_after(to, from, flen);

	FDBFuture *f = fdb_transaction_get_range(tr, FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(from, flen),
	                                         FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(to, tlen), 0, 0,
	                                         FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);
	fdb_error_t err = await(f);
	if (!err) {
		const FDBKeyValue *kv;
		int count;
		fdb_bool_t more;
		err = fdb_future_get_keyvalue_array(f, &kv, &count, &more);
		if (!err) {
			for (int i = 0; i < count && c->nparts < TXN_MAX_PARTS; i++) {
				if (kv[i].value_length != 16) continue;
				TxnPart *p = &c->parts[c->nparts];
				int n = kv[i].key_length - flen;
				if (n <= 0 || n >= MAX_NAME) continue;
				memcpy(p->name, kv[i].key + flen, (size_t)n);
				p->name[n] = '\0';
				p->txid = get_be64(kv[i].value);
				p->size = get_be64(kv[i].value + 8);
				c->nparts++;
			}
		}
	}
	fdb_future_destroy(f);
	return err;
}

struct intent_ctx {
	const TxnPart *part;
	int present;
	// Whether this participant's fence must go up. Only a missing intent needs it.
	int fence_it;
	uint64_t head;
	uint32_t pgno[4096];
	int npages;
};

// QueryIntent. A participant staged in one transaction, so its pages are all there or
// none are, and one row answers it. The page numbers come back too, because resolving
// needs them to write the index.
//
// A participant that wrote nothing staged at its own head, and its intent is trivially
// present: there is nothing that could be missing.
static fdb_error_t query_intent_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct intent_ctx *c = ctx;
	uint8_t key[KEYMAX], from[KEYMAX], to[KEYMAX];
	int got = 0;

	c->present = 0;
	c->npages = 0;

	int klen = key_meta(key, c->part->name, "HEAD");
	fdb_error_t err = get_u64(tr, key, klen, &c->head, &got);
	if (err) return err;
	if (!got) c->head = 0;

	if (c->part->txid <= c->head) {
		// Already resolved, or the participant wrote nothing. Either way there is no
		// intent left to be missing.
		c->present = 1;
		return 0;
	}

	int flen = key_delta_txid(from, c->part->name, c->part->txid);
	int tlen = key_after(to, from, flen);
	FDBFuture *f = fdb_transaction_get_range(tr, FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(from, flen),
	                                         FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(to, tlen), 0, 0,
	                                         FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);
	err = await(f);
	if (!err) {
		const FDBKeyValue *kv;
		int count;
		fdb_bool_t more;
		err = fdb_future_get_keyvalue_array(f, &kv, &count, &more);
		if (!err) {
			for (int i = 0; i < count && c->npages < 4096; i++) {
				if (kv[i].key_length < 4) continue;
				const uint8_t *p = kv[i].key + kv[i].key_length - 4;
				c->pgno[c->npages++] = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
				                       | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
			}
			c->present = count > 0;
		}
	}
	fdb_future_destroy(f);
	return err;
}

// Resolve one participant from the record alone: point its index at the staged pages,
// move its size and its head. This is `put_head` without an FdbFile, because recovery has
// no open handle to work from.
static fdb_error_t resolve_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct intent_ctx *c = ctx;
	uint8_t key[KEYMAX];

	if (c->part->txid <= c->head) return 0; // somebody resolved it first

	for (int i = 0; i < c->npages; i++) {
		int klen = key_pidx(key, c->part->name, c->pgno[i]);
		set_u64(tr, key, klen, c->part->txid);
	}

	int klen = key_meta(key, c->part->name, "SIZE");
	set_u64(tr, key, klen, c->part->size);

	uint64_t logn = 0;
	int got = 0;
	klen = key_meta(key, c->part->name, "LOGN");
	fdb_error_t err = get_u64(tr, key, klen, &logn, &got);
	if (err) return err;
	set_u64(tr, key, klen, (got ? logn : 0) + (uint64_t)c->npages);

	klen = key_meta(key, c->part->name, "HEAD");
	set_u64(tr, key, klen, c->part->txid);
	return 0;
}

// Give up one participant of an aborted group. Two jobs, and conflating them is the fault.
//
// The pages always go: they are unreachable and nothing will ever point at them.
//
// The fence goes up only for the participant whose intent was missing, which is the write
// that must never land. ParallelCommits.tla bumps the timestamp cache for the key it failed
// to find and for no other, and the difference is not cosmetic: raising the fence on a
// healthy participant invalidates every live handle on a database that had nothing wrong
// with it. Measured on a record with one present intent and one missing, the healthy
// participant's fence went from 1 to 2 before this, and stays at 1 now.
static fdb_error_t prevent_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct intent_ctx *c = ctx;
	uint8_t key[KEYMAX], from[KEYMAX], to[KEYMAX];

	if (c->fence_it) {
		uint64_t fence = 0;
		int got = 0;
		int klen = key_meta(key, c->part->name, "FENCE");
		fdb_error_t err = get_u64(tr, key, klen, &fence, &got);
		if (err) return err;
		set_u64(tr, key, klen, got ? fence + 1 : 1);
	}

	// The staged pages are unreachable now and nothing will ever point at them.
	int flen = key_delta_txid(from, c->part->name, c->part->txid);
	int tlen = key_after(to, from, flen);
	fdb_transaction_clear_range(tr, from, flen, to, tlen);
	return 0;
}

// Decide one staging record and carry the decision out.
static int recover_one(uint64_t txnid) {
	struct recover_ctx c = {txnid, {{{0}, 0, 0}}, 0, 1};
	int rc = run_txn(read_parts_body, &c, 0, SQLITE_IOERR_READ);
	if (rc != SQLITE_OK) return rc;
	if (c.nparts == 0) {
		// A record with no participants decides nothing and cannot be committed.
		struct status_ctx s = {txnid, TXN_ABORTED};
		return run_txn(drop_record_body, &s, 1, SQLITE_IOERR_WRITE);
	}

	// Query every intent before changing anything, because the decision depends on all of
	// them and a partial answer is the one thing that must not drive it.
	struct intent_ctx intents[TXN_MAX_PARTS];
	for (int i = 0; i < c.nparts; i++) {
		intents[i].part = &c.parts[i];
		rc = run_txn(query_intent_body, &intents[i], 0, SQLITE_IOERR_READ);
		if (rc != SQLITE_OK) return rc;
		if (!intents[i].present) c.all_present = 0;
	}

	if (c.all_present) {
		// Implicitly committed. Finishing it is not a choice.
		for (int i = 0; i < c.nparts; i++) {
			rc = run_txn(resolve_body, &intents[i], 1, SQLITE_IOERR_WRITE);
			if (rc != SQLITE_OK) return rc;
		}
		struct status_ctx s = {txnid, TXN_COMMITTED};
		rc = run_txn(set_status_body, &s, 1, SQLITE_IOERR_WRITE);
		if (rc != SQLITE_OK) return rc;
		return run_txn(drop_record_body, &s, 1, SQLITE_IOERR_WRITE);
	}

	for (int i = 0; i < c.nparts; i++) {
		intents[i].fence_it = !intents[i].present;
		rc = run_txn(prevent_body, &intents[i], 1, SQLITE_IOERR_WRITE);
		if (rc != SQLITE_OK) return rc;
	}
	struct status_ctx s = {txnid, TXN_ABORTED};
	rc = run_txn(set_status_body, &s, 1, SQLITE_IOERR_WRITE);
	if (rc != SQLITE_OK) return rc;
	return run_txn(drop_record_body, &s, 1, SQLITE_IOERR_WRITE);
}

struct sweep_ctx {
	uint64_t staging[TXN_MAX_RECOVER];
	int n;
};

// Every record still in the staging state. `weft/txn/NEXT` sorts under the same prefix and
// is not a record, so it is skipped by shape: a record's key carries an 8 byte txid.
static fdb_error_t sweep_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct sweep_ctx *c = ctx;
	uint8_t from[KEYMAX], to[KEYMAX];

	c->n = 0;
	int flen = key_txn_all(from);
	int tlen = key_after(to, from, flen);

	FDBFuture *f = fdb_transaction_get_range(tr, FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(from, flen),
	                                         FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(to, tlen), 0, 0,
	                                         FDB_STREAMING_MODE_WANT_ALL, 0, 0, 0);
	fdb_error_t err = await(f);
	if (!err) {
		const FDBKeyValue *kv;
		int count;
		fdb_bool_t more;
		err = fdb_future_get_keyvalue_array(f, &kv, &count, &more);
		if (!err) {
			const int suffix = 7; // "/STATUS"
			for (int i = 0; i < count && c->n < TXN_MAX_RECOVER; i++) {
				if (kv[i].key_length != flen + 8 + suffix) continue;
				if (memcmp(kv[i].key + flen + 8, "/STATUS", (size_t)suffix) != 0) continue;
				if (kv[i].value_length != 1 || kv[i].value[0] != TXN_STAGING) continue;
				c->staging[c->n++] = get_be64(kv[i].key + flen);
			}
		}
	}
	fdb_future_destroy(f);
	return err;
}

// Decide every staging record.
//
// `vfs_open` calls this before it raises a fence, which is what makes the ordering
// structural rather than remembered. It stays public because a process may want to settle
// what a crash left before it opens anything at all, and because recovery is a thing a
// caller can reasonably ask for on its own.
//
// Safe to call from more than one thread and more than one process: every outcome it can
// reach is idempotent, which is the property that lets any finder of a record decide it.
int weft_txn_recover(void) {
	for (;;) {
		struct sweep_ctx c = {{0}, 0};
		int rc = run_txn(sweep_body, &c, 0, SQLITE_IOERR_READ);
		if (rc != SQLITE_OK) return rc;
		if (c.n == 0) return SQLITE_OK;

		for (int i = 0; i < c.n; i++) {
			rc = recover_one(c.staging[i]);
			if (rc != SQLITE_OK) return rc;
		}
		// A full sweep may have been truncated, so go round again until one comes back
		// empty. Each pass strictly removes records, so this ends.
		if (c.n < TXN_MAX_RECOVER) return SQLITE_OK;
	}
}

// ── The rest of the file methods ──────────────────────────────────────────────

static int fdb_truncate(sqlite3_file *file, sqlite3_int64 size) {
	FdbFile *f = (FdbFile *)file;
	int64_t npages = (size + PAGE - 1) / PAGE;

	// Drop the buffered pages the truncate removes.
	int slot;
	find_dirty(f, (uint32_t)npages, &slot);
	for (int i = slot; i < f->ndirty; i++) free(f->dirty[i]);
	f->ndirty = slot;

	if (f->trunc_pages < 0 || npages < f->trunc_pages) f->trunc_pages = npages;
	f->size = size;
	ra_reset(f);
	return SQLITE_OK;
}

// SQLite syncs the database when a commit is complete, so this is where the commit goes
// to FoundationDB. A commit is durable when FoundationDB commits it, so there is nothing
// else for a sync to do.
static int fdb_sync(sqlite3_file *file, int flags) {
	(void)flags;
	return flush((FdbFile *)file);
}

static int fdb_file_size(sqlite3_file *file, sqlite3_int64 *out) {
	*out = ((FdbFile *)file)->size;
	return SQLITE_OK;
}

static int fdb_close(sqlite3_file *file) {
	FdbFile *f = (FdbFile *)file;
	int rc = flush(f);

	// A fold owed at close still gets paid, so a caller that never asks is no worse off
	// than before this was deferred. It is off the commit path either way, which is the
	// point: closing is already the slow moment.
	if (rc == SQLITE_OK && f->compact_due && !f->group_txnid) {
		f->compact_due = 0;
		rc = compact(f);
	}
	clear_dirty(f);
	free(f->dirty);
	f->dirty = NULL;
	f->capdirty = 0;
	if (f->ro_tr) {
		fdb_transaction_destroy(f->ro_tr);
		f->ro_tr = NULL;
	}
	free(f->bases);
	f->bases = NULL;
	free(f->ra_window);
	free(f->ra_state);
	f->ra_window = NULL;
	f->ra_state = NULL;
	f->ra_count = 0;
	return rc;
}

// The actor owns its store and is the single writer, so a lock is not needed.
static int fdb_lock(sqlite3_file *f, int l) { (void)f; (void)l; return SQLITE_OK; }

// Under NORMAL locking SQLite drops to no lock at the end of every transaction, and that is
// the moment a handle following the head wants a fresh read version: the next transaction
// then starts a new FoundationDB transaction, `check_view` runs, and a moved head is seen
// now rather than when the cached transaction's four seconds run out. Under EXCLUSIVE
// locking this is never called, and the cache lives as before.
static int fdb_unlock(sqlite3_file *file, int l) {
	FdbFile *f = (FdbFile *)file;
	if (l == SQLITE_LOCK_NONE) {
		f->cache_droppable = 1;
		if (f->ro_tr) {
			fdb_transaction_destroy(f->ro_tr);
			f->ro_tr = NULL;
		}
	}
	return SQLITE_OK;
}
static int fdb_check_lock(sqlite3_file *f, int *out) { (void)f; *out = 0; return SQLITE_OK; }

static int fdb_control(sqlite3_file *file, int op, void *arg) {
	(void)arg;
	// SQLite reports the end of a commit here. A connection with synchronous off never
	// syncs, so without this the commit would sit in memory.
	if (op == SQLITE_FCNTL_COMMIT_PHASETWO) return flush((FdbFile *)file);
	return SQLITE_NOTFOUND;
}

static int fdb_sector(sqlite3_file *f) { (void)f; return PAGE; }

static int fdb_devchar(sqlite3_file *f) {
	(void)f;
	// One page reaches FoundationDB whole, and a commit reaches it as one transaction.
	// So SQLite may skip the work it would otherwise do to survive a torn page.
	return SQLITE_IOCAP_ATOMIC4K | SQLITE_IOCAP_SAFE_APPEND | SQLITE_IOCAP_SEQUENTIAL;
}

static const sqlite3_io_methods FDB_IO = {
	1, fdb_close, fdb_read, fdb_write, fdb_truncate, fdb_sync, fdb_file_size,
	fdb_lock, fdb_unlock, fdb_check_lock, fdb_control, fdb_sector, fdb_devchar,
};

// The method table is the identity of the VFS, so comparing against it is how a file is
// known to be one of ours. Declared above, where the group commit needs it.
static int is_fdb_file(const sqlite3_file *file) {
	return file && file->pMethods == &FDB_IO;
}

// ── The VFS ───────────────────────────────────────────────────────────────────

static int vfs_open(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags,
                    int *out_flags) {
	(void)vfs;
	FdbFile *f = (FdbFile *)file;
	memset(f, 0, sizeof(*f));
	f->base.pMethods = &FDB_IO;
	f->trunc_pages = -1;
	snprintf(f->name, MAX_NAME, "%s", name ? name : "anonymous");

	// The real cost of an index row for this name, measured rather than bounded. The page
	// number is irrelevant to the length — `key_pidx` always writes four bytes for it — so
	// any page number gives the answer.
	{
		uint8_t probe[KEYMAX];
		f->pidx_row = key_pidx(probe, f->name, 0) + 8;
	}

	// Decide every staging group before this open raises a fence.
	//
	// This ordering is not a preference. Opening raises the fence, and a fence raised over a
	// group that is already implicitly committed prevents a commit that has happened: the
	// staged pages become unreachable and recovery, finding an intent it can no longer
	// reach, aborts a group the protocol says must be finished. The write was acknowledged
	// and is then destroyed, with nothing anywhere reporting it.
	//
	// It used to be a rule in a comment that callers obeyed by hand. A rule that holds only
	// while somebody remembers it is the kind of fault the rest of this suite exists to
	// catch, so it lives here now, where it cannot be got wrong.
	//
	// The cost is one range read for each open. Records are dropped as they are decided, so
	// in the steady state that read finds nothing and the sweep stops.
	const int readonly = (flags & SQLITE_OPEN_READONLY) != 0;
	f->readonly = readonly;
	f->cache_droppable = 1; // nothing is cached yet
	int rc = readonly ? SQLITE_OK : weft_txn_recover();
	if (rc != SQLITE_OK) return rc;

	struct open_ctx c = {f, readonly};
	rc = run_txn(open_body, &c, 1, SQLITE_IOERR);
	if (rc != SQLITE_OK) return rc;

	if (out_flags) *out_flags = flags;
	return SQLITE_OK;
}

struct delete_ctx {
	const char *name;
};

static fdb_error_t delete_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	const char *name = ((struct delete_ctx *)ctx)->name;
	uint8_t from[KEYMAX], to[KEYMAX];
	int plen = snprintf((char *)from, KEYMAX, "weft/db/%s/", name);
	int tlen = key_after(to, from, plen);
	fdb_transaction_clear_range(tr, from, plen, to, tlen);
	return 0;
}

static int vfs_delete(sqlite3_vfs *vfs, const char *name, int sync) {
	(void)vfs;
	(void)sync;
	struct delete_ctx c = {name};
	return run_txn(delete_body, &c, 1, SQLITE_IOERR_DELETE);
}

struct access_ctx {
	const char *name;
	int exists;
};

static fdb_error_t access_body(FDBTransaction *tr, void *ctx, int *final) {
	(void)final;
	struct access_ctx *a = ctx;
	uint8_t key[KEYMAX];
	uint64_t size = 0;
	int got = 0;
	int klen = key_meta(key, a->name, "SIZE");
	fdb_error_t err = get_u64(tr, key, klen, &size, &got);
	if (err) return err;
	a->exists = got && size > 0;
	return 0;
}

static int vfs_access(sqlite3_vfs *vfs, const char *name, int flags, int *out) {
	(void)vfs;
	(void)flags;
	struct access_ctx c = {name, 0};
	int rc = run_txn(access_body, &c, 0, SQLITE_IOERR);
	*out = rc == SQLITE_OK ? c.exists : 0;
	return SQLITE_OK;
}

static int vfs_fullpath(sqlite3_vfs *vfs, const char *in, int n, char *out) {
	(void)vfs;
	snprintf(out, (size_t)n, "%s", in);
	return SQLITE_OK;
}

// Randomness, sleep and the clock come from the VFS that was the default when this one
// registered: "unix" is absent on Windows, and a SQLite built with SQLITE_OMIT_DEPRECATED
// (exqlite's) has no xCurrentTime, only xCurrentTimeInt64.
static sqlite3_vfs *g_base;

// `WEFT_PLANT_UNIX_CLOCK=1` plants the defect this replaced: the clock read from "unix"'s
// xCurrentTime. `prove_clock` runs under it to show the fixture fails on that VFS.
static int plant_unix_clock(void) {
	const char *v = getenv("WEFT_PLANT_UNIX_CLOCK");
	return v && *v && strcmp(v, "0") != 0;
}

static int vfs_randomness(sqlite3_vfs *vfs, int n, char *out) {
	(void)vfs;
	return g_base->xRandomness(g_base, n, out);
}
static int vfs_sleep(sqlite3_vfs *vfs, int micros) {
	(void)vfs;
	return g_base->xSleep(g_base, micros);
}
static int vfs_time64(sqlite3_vfs *vfs, sqlite3_int64 *out) {
	(void)vfs;
	double r = 0;
	int rc;
	if (plant_unix_clock()) {
		rc = sqlite3_vfs_find("unix")->xCurrentTime(vfs, &r);
	} else if (g_base->iVersion >= 2 && g_base->xCurrentTimeInt64) {
		return g_base->xCurrentTimeInt64(g_base, out);
	} else {
		rc = g_base->xCurrentTime(g_base, &r);
	}
	*out = (sqlite3_int64)(r * 86400000.0);
	return rc;
}
static int vfs_time(sqlite3_vfs *vfs, double *out) {
	sqlite3_int64 t = 0;
	int rc = vfs_time64(vfs, &t);
	*out = t / 86400000.0;
	return rc;
}

static sqlite3_vfs FDB_VFS = {
	.iVersion = 2,
	.szOsFile = sizeof(FdbFile),
	.mxPathname = MAX_NAME,
	.zName = "weft_fdb",
	.xOpen = vfs_open,
	.xDelete = vfs_delete,
	.xAccess = vfs_access,
	.xFullPathname = vfs_fullpath,
	.xRandomness = vfs_randomness,
	.xSleep = vfs_sleep,
	.xCurrentTime = vfs_time,
	.xCurrentTimeInt64 = vfs_time64,
};

int weft_vfs_register(int make_default) {
	if (!g_base) g_base = sqlite3_vfs_find(0);
	if (!g_base || g_base == &FDB_VFS) return SQLITE_ERROR;
	return sqlite3_vfs_register(&FDB_VFS, make_default);
}
