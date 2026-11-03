// The keys of the store layout.
//
// This is layer 1 of the four `fdb_vfs.c` describes, split out so it can be tested
// without FoundationDB or SQLite. Nothing here allocates, does I/O, or holds state: a key
// is bytes in a caller's buffer, and every function returns the length it wrote.
//
// A number goes into a key big endian, so the order of the keys is the order of the
// numbers. That is not a convenience. `edge_number` finds the newest shard version and
// the oldest pin by reading the edge of a range, `load_newest_shard` trusts that order,
// and `drop_unfinished_commit` clears a range that starts at a number. If the encoding
// ever stopped being order preserving, each of those would read the wrong row and none of
// them would fail loudly. `keys-witness/keys_test.cc` holds that property and the ones below.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef WEFT_FDB_KEYS_H
#define WEFT_FDB_KEYS_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// The largest key this layout builds. A name is bounded, and every suffix is a fixed
// number of bytes, so this bounds them all.
#define KEYMAX 640

static inline void put_be32(uint8_t *out, uint32_t v) {
	for (int i = 0; i < 4; i++) out[i] = (uint8_t)(v >> (24 - 8 * i));
}

static inline void put_be64(uint8_t *out, uint64_t v) {
	for (int i = 0; i < 8; i++) out[i] = (uint8_t)(v >> (56 - 8 * i));
}

static inline uint64_t get_be64(const uint8_t *in) {
	uint64_t v = 0;
	for (int i = 0; i < 8; i++) v = (v << 8) | in[i];
	return v;
}

static inline int key_meta(uint8_t *out, const char *name, const char *what) {
	return snprintf((char *)out, KEYMAX, "weft/db/%s/%s", name, what);
}

static inline int key_pidx(uint8_t *out, const char *name, uint32_t pgno) {
	int n = snprintf((char *)out, KEYMAX, "weft/db/%s/PIDX/", name);
	put_be32(out + n, pgno);
	return n + 4;
}

static inline int key_delta(uint8_t *out, const char *name, uint64_t txid, uint32_t pgno) {
	int n = snprintf((char *)out, KEYMAX, "weft/db/%s/DELTA/", name);
	put_be64(out + n, txid);
	put_be32(out + n + 8, pgno);
	return n + 12;
}

static inline int key_delta_txid(uint8_t *out, const char *name, uint64_t txid) {
	int n = snprintf((char *)out, KEYMAX, "weft/db/%s/DELTA/", name);
	put_be64(out + n, txid);
	return n + 8;
}

static inline int key_shard(uint8_t *out, const char *name, uint64_t as_of, uint32_t pgno) {
	int n = snprintf((char *)out, KEYMAX, "weft/db/%s/SHARD/", name);
	put_be64(out + n, as_of);
	put_be32(out + n + 8, pgno);
	return n + 12;
}

static inline int key_shard_version(uint8_t *out, const char *name, uint64_t as_of) {
	int n = snprintf((char *)out, KEYMAX, "weft/db/%s/SHARD/", name);
	put_be64(out + n, as_of);
	return n + 8;
}

static inline int key_shardn(uint8_t *out, const char *name, uint64_t as_of) {
	int n = snprintf((char *)out, KEYMAX, "weft/db/%s/SHARDN/", name);
	put_be64(out + n, as_of);
	return n + 8;
}

static inline int key_prefix(uint8_t *out, const char *name, const char *what) {
	return snprintf((char *)out, KEYMAX, "weft/db/%s/%s/", name, what);
}

// ── The transaction record ────────────────────────────────────────────────────
//
// A commit that spans several databases cannot be one database's transaction, and
// `flush` is scoped to one file. So a group of them shares a record, which is what
// ParallelCommits.tla calls the transaction record:
//
//   weft/txn/<txnid>/STATUS        staging, committed, or aborted
//   weft/txn/<txnid>/PART/<name>   a participant, and the txid it staged under
//
// The record lives outside `weft/db/`, because it belongs to no single database. A
// participant row names a database and the staged txid its pages went under, which is the
// only thing recovery needs to find them again.

// Status values. They are written as one byte, because the set is closed and a string
// would invite a fifth state that the state machine does not have.
#define TXN_STAGING 1
#define TXN_COMMITTED 2
#define TXN_ABORTED 3

static inline int key_txn_status(uint8_t *out, uint64_t txnid) {
	int n = snprintf((char *)out, KEYMAX, "weft/txn/");
	put_be64(out + n, txnid);
	return n + 8 + snprintf((char *)out + n + 8, KEYMAX - n - 8, "/STATUS");
}

static inline int key_txn_part(uint8_t *out, uint64_t txnid, const char *name) {
	int n = snprintf((char *)out, KEYMAX, "weft/txn/");
	put_be64(out + n, txnid);
	return n + 8 + snprintf((char *)out + n + 8, KEYMAX - n - 8, "/PART/%s", name);
}

static inline int key_txn_part_prefix(uint8_t *out, uint64_t txnid) {
	int n = snprintf((char *)out, KEYMAX, "weft/txn/");
	put_be64(out + n, txnid);
	return n + 8 + snprintf((char *)out + n + 8, KEYMAX - n - 8, "/PART/");
}

static inline int key_txn_prefix(uint8_t *out, uint64_t txnid) {
	int n = snprintf((char *)out, KEYMAX, "weft/txn/");
	put_be64(out + n, txnid);
	return n + 8;
}

// Every transaction record, so recovery can sweep for the staging ones.
static inline int key_txn_all(uint8_t *out) {
	return snprintf((char *)out, KEYMAX, "weft/txn/");
}

// The first key after every key with this prefix. FoundationDB calls it strinc. A range
// from a prefix to this covers exactly the keys under that prefix.
static inline int key_after(uint8_t *out, const uint8_t *prefix, int plen) {
	if (out != prefix) memcpy(out, prefix, (size_t)plen);
	int n = plen;
	while (n > 0 && out[n - 1] == 0xFF) n--;
	if (n > 0) out[n - 1]++;
	return n;
}

#endif
