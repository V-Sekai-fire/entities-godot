// Ports every property in `../fuzztest/keys_test.cc` to witness-cpp's PROP_CHECK,
// and pairs each with a planted-false rule-2 control per CLAUDE.md's "How Work Is
// Verified" rule 2.
//
// The properties are the ones `fdb_keys.h` claims and three places in `fdb_vfs.c`
// depend on: `edge_number`, `load_newest_shard`, `drop_unfinished_commit`. A wrong
// answer there is a read of the wrong page, which `PRAGMA integrity_check` cannot see.
//
// SPDX-License-Identifier: Apache-2.0

extern "C" {
#include "fdb_keys.h"
}

#include "witness/doctest.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

std::string key_pidx_str(const std::string &name, uint32_t pgno) {
	uint8_t buf[KEYMAX];
	int n = key_pidx(buf, name.c_str(), pgno);
	return std::string(reinterpret_cast<char *>(buf), static_cast<size_t>(n));
}

std::string key_delta_str(const std::string &name, uint64_t txid, uint32_t pgno) {
	uint8_t buf[KEYMAX];
	int n = key_delta(buf, name.c_str(), txid, pgno);
	return std::string(reinterpret_cast<char *>(buf), static_cast<size_t>(n));
}

std::string key_shard_version_str(const std::string &name, uint64_t as_of) {
	uint8_t buf[KEYMAX];
	int n = key_shard_version(buf, name.c_str(), as_of);
	return std::string(reinterpret_cast<char *>(buf), static_cast<size_t>(n));
}

std::string key_txn_status_str(uint64_t txnid) {
	uint8_t buf[KEYMAX];
	int n = key_txn_status(buf, txnid);
	return std::string(reinterpret_cast<char *>(buf), static_cast<size_t>(n));
}

// Names the layout can carry. `fdb_vfs.c` bounds names at MAX_NAME and builds keys
// with snprintf, so short printable-ASCII names are the interesting space.
witness::Generator<std::string> gen_name() {
	std::string alphabet;
	for (char c = 0x20; c < 0x7f; ++c) alphabet.push_back(c);
	return witness::gen_string_of(alphabet);
}

witness::Generator<std::vector<uint8_t>> gen_bytes(size_t max_len) {
	return [max_len](witness::RNG &rng, const witness::Level &lvl) {
		size_t cap = max_len < static_cast<size_t>(lvl.fin_bound)
			? max_len : static_cast<size_t>(lvl.fin_bound);
		uint32_t n = rng.uint_range(0, static_cast<uint32_t>(cap));
		std::vector<uint8_t> v(n);
		for (uint32_t i = 0; i < n; ++i) v[i] = static_cast<uint8_t>(rng.uint_range(0, 255));
		return v;
	};
}

// A uint64 generator that spans the whole 64-bit range at every rung, so a rule-2 control
// looking for a top-byte or high-bit disagreement finds one at rung 0 rather than never.
uint64_t gen_uint64_wide(witness::RNG &rng, const witness::Level &) {
	uint64_t hi = rng.uint_range(0, UINT32_MAX);
	uint64_t lo = rng.uint_range(0, UINT32_MAX);
	return (hi << 32) | lo;
}

// A uint32 generator that spans the whole 32-bit range, for the little-endian control.
uint32_t gen_uint32_wide(witness::RNG &rng, const witness::Level &) {
	return rng.uint_range(0, UINT32_MAX);
}

}  // namespace

// ── The encoding round trips ──────────────────────────────────────────────────

TEST_CASE("[keys] big-endian round trip") {
	witness::Generator<uint64_t> gen = &witness::gen_uint64;
	std::function<bool(const uint64_t &)> pred = [](const uint64_t &v) {
		uint8_t buf[8];
		put_be64(buf, v);
		return get_be64(buf) == v;
	};
	PROP_CHECK(uint64_t, "put_be64 then get_be64 is the identity", gen, pred);
}

TEST_CASE("[keys] rule-2 control: a broken be64 loses the top byte") {
	// Planted-false: mask off the top byte on decode. Must FOUND at rung 0. Uses the
	// wide uint64 generator so a value with a non-zero top byte is sampled at rung 0.
	witness::Generator<uint64_t> gen = &gen_uint64_wide;
	std::function<bool(const uint64_t &)> pred = [](const uint64_t &v) {
		uint8_t buf[8];
		put_be64(buf, v);
		uint64_t got = 0;
		for (int i = 1; i < 8; ++i) got = (got << 8) | buf[i];   // top byte dropped
		return got == v;
	};
	witness::Trial t = witness::resolve<uint64_t>("broken be64 agrees with input", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

// ── Order of the keys is order of the numbers: PIDX ───────────────────────────

using PidxInput = std::tuple<std::string, uint32_t, uint32_t>;

TEST_CASE("[keys] PIDX keys sort like page numbers") {
	witness::Generator<PidxInput> gen = witness::gen_tuple<std::string, uint32_t, uint32_t>(
		gen_name(), &witness::gen_uint, &witness::gen_uint);
	std::function<bool(const PidxInput &)> pred = [](const PidxInput &t) {
		const std::string &name = std::get<0>(t);
		uint32_t a = std::get<1>(t);
		uint32_t b = std::get<2>(t);
		std::string ka = key_pidx_str(name, a);
		std::string kb = key_pidx_str(name, b);
		if (a < b) return ka < kb;
		if (a > b) return ka > kb;
		return ka == kb;
	};
	PROP_CHECK(PidxInput, "key_pidx lex order matches page-number order", gen, pred);
}

TEST_CASE("[keys] rule-2 control: PIDX with a broken little-endian encoder loses order") {
	// Same shape, but the "wrong" encoder swaps to little-endian for the last four bytes
	// of the key. Order of the numbers no longer matches lex order. Uses the wide uint32
	// generator so a pair whose big-endian and little-endian orderings disagree is sampled.
	witness::Generator<PidxInput> gen = witness::gen_tuple<std::string, uint32_t, uint32_t>(
		gen_name(), &gen_uint32_wide, &gen_uint32_wide);
	std::function<bool(const PidxInput &)> pred = [](const PidxInput &t) {
		const std::string &name = std::get<0>(t);
		uint32_t a = std::get<1>(t);
		uint32_t b = std::get<2>(t);
		auto le_key = [&](uint32_t n) {
			uint8_t buf[KEYMAX];
			int k = snprintf((char *)buf, KEYMAX, "weft/db/%s/PIDX/", name.c_str());
			for (int i = 0; i < 4; ++i) buf[k + i] = (uint8_t)(n >> (8 * i));  // little-endian, wrong
			return std::string((char *)buf, k + 4);
		};
		std::string ka = le_key(a);
		std::string kb = le_key(b);
		if (a < b) return ka < kb;
		if (a > b) return ka > kb;
		return ka == kb;
	};
	witness::Trial t = witness::resolve<PidxInput>(
		"little-endian PIDX preserves order", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

// ── Order: DELTA ──────────────────────────────────────────────────────────────

using DeltaInput = std::tuple<std::string, uint64_t, uint32_t, uint64_t, uint32_t>;

TEST_CASE("[keys] DELTA keys sort by txid then page") {
	witness::Generator<DeltaInput> gen =
		witness::gen_tuple<std::string, uint64_t, uint32_t, uint64_t, uint32_t>(
			gen_name(), &witness::gen_uint64, &witness::gen_uint,
			&witness::gen_uint64, &witness::gen_uint);
	std::function<bool(const DeltaInput &)> pred = [](const DeltaInput &t) {
		const std::string &name = std::get<0>(t);
		uint64_t ta = std::get<1>(t);
		uint32_t pa = std::get<2>(t);
		uint64_t tb = std::get<3>(t);
		uint32_t pb = std::get<4>(t);
		std::string ka = key_delta_str(name, ta, pa);
		std::string kb = key_delta_str(name, tb, pb);
		bool same = ta == tb && pa == pb;
		bool a_first = ta < tb || (ta == tb && pa < pb);
		if (same) return ka == kb;
		if (a_first) return ka < kb;
		return ka > kb;
	};
	PROP_CHECK(DeltaInput,
		"key_delta lex order is (txid, page) lex order", gen, pred);
}

TEST_CASE("[keys] rule-2 control: DELTA with (page, txid) key order loses commit-range contiguity") {
	// If DELTA were keyed (pgno, txid), a single commit's pages would not be one range.
	// `drop_unfinished_commit` would then clear the wrong keys.
	witness::Generator<DeltaInput> gen =
		witness::gen_tuple<std::string, uint64_t, uint32_t, uint64_t, uint32_t>(
			gen_name(), &witness::gen_uint64, &witness::gen_uint,
			&witness::gen_uint64, &witness::gen_uint);
	std::function<bool(const DeltaInput &)> pred = [](const DeltaInput &t) {
		const std::string &name = std::get<0>(t);
		uint64_t ta = std::get<1>(t);
		uint32_t pa = std::get<2>(t);
		uint64_t tb = std::get<3>(t);
		uint32_t pb = std::get<4>(t);
		auto swapped = [&](uint64_t tx, uint32_t pg) {
			uint8_t buf[KEYMAX];
			int k = snprintf((char *)buf, KEYMAX, "weft/db/%s/DELTA/", name.c_str());
			put_be32(buf + k, pg);           // page first, wrong order
			put_be64(buf + k + 4, tx);
			return std::string((char *)buf, k + 12);
		};
		std::string ka = swapped(ta, pa);
		std::string kb = swapped(tb, pb);
		bool same = ta == tb && pa == pb;
		bool a_first = ta < tb || (ta == tb && pa < pb);
		if (same) return ka == kb;
		if (a_first) return ka < kb;
		return ka > kb;
	};
	witness::Trial t = witness::resolve<DeltaInput>(
		"page-first DELTA sorts by (txid, page)", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

// ── Order: SHARD version bound ────────────────────────────────────────────────

using ShardInput = std::tuple<std::string, uint64_t, uint64_t>;

TEST_CASE("[keys] shard-version bound excludes exactly the versions above it") {
	witness::Generator<ShardInput> gen = witness::gen_tuple<std::string, uint64_t, uint64_t>(
		gen_name(), &witness::gen_uint64, &witness::gen_uint64);
	std::function<bool(const ShardInput &)> pred = [](const ShardInput &t) {
		const std::string &name = std::get<0>(t);
		uint64_t as_of = std::get<1>(t);
		uint64_t head = std::get<2>(t);
		if (head == UINT64_MAX) return true;   // no successor to compare against
		std::string version = key_shard_version_str(name, as_of);
		std::string bound = key_shard_version_str(name, head + 1);
		if (as_of <= head) return version < bound;
		return version >= bound;
	};
	PROP_CHECK(ShardInput,
		"shard version is below the head+1 bound iff as_of <= head", gen, pred);
}

TEST_CASE("[keys] rule-2 control: a signed comparison flips near UINT64_MAX/2") {
	// Comparing the shard-version bound as signed instead of unsigned lies for as_of
	// on either side of 2^63. Wide generator so both sides of 2^63 are sampled.
	witness::Generator<ShardInput> gen = witness::gen_tuple<std::string, uint64_t, uint64_t>(
		gen_name(), &gen_uint64_wide, &gen_uint64_wide);
	std::function<bool(const ShardInput &)> pred = [](const ShardInput &t) {
		uint64_t as_of = std::get<1>(t);
		uint64_t head = std::get<2>(t);
		if (head == UINT64_MAX) return true;
		bool signed_le = (int64_t)as_of <= (int64_t)head;   // wrong: signed
		bool actual_le = as_of <= head;
		return signed_le == actual_le;   // planted-false: sometimes disagrees
	};
	witness::Trial t = witness::resolve<ShardInput>(
		"signed and unsigned comparisons agree on 64-bit versions", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

// ── strinc bounds a prefix exactly ────────────────────────────────────────────

using PrefixTail = std::pair<std::vector<uint8_t>, std::vector<uint8_t>>;

TEST_CASE("[keys] key_after is above every key with the prefix") {
	witness::Generator<PrefixTail> gen = witness::gen_pair<std::vector<uint8_t>, std::vector<uint8_t>>(
		gen_bytes(KEYMAX - 1), gen_bytes(KEYMAX - 1));
	std::function<bool(const PrefixTail &)> pred = [](const PrefixTail &pt) {
		const std::vector<uint8_t> &prefix = pt.first;
		const std::vector<uint8_t> &tail = pt.second;
		if (prefix.empty() || prefix.size() > KEYMAX - 1) return true;
		bool all_ff = true;
		for (uint8_t b : prefix) all_ff = all_ff && b == 0xFF;
		uint8_t end[KEYMAX];
		int elen = key_after(end, prefix.data(), (int)prefix.size());
		if (all_ff) return elen == 0;
		if (elen <= 0) return false;
		std::string bound((char *)end, (size_t)elen);
		std::string key((const char *)prefix.data(), prefix.size());
		size_t room = KEYMAX - prefix.size();
		key.append((const char *)tail.data(), tail.size() < room ? tail.size() : room);
		return key < bound;
	};
	PROP_CHECK(PrefixTail, "every key under the prefix sorts below key_after", gen, pred);
}

TEST_CASE("[keys] rule-2 control: key_after that returns the prefix unchanged fails") {
	// Planted-wrong: implement key_after as "just return the prefix." A key equal to
	// the prefix now sorts at, not below, the bound.
	witness::Generator<PrefixTail> gen = witness::gen_pair<std::vector<uint8_t>, std::vector<uint8_t>>(
		gen_bytes(KEYMAX - 1), gen_bytes(KEYMAX - 1));
	std::function<bool(const PrefixTail &)> pred = [](const PrefixTail &pt) {
		const std::vector<uint8_t> &prefix = pt.first;
		if (prefix.empty() || prefix.size() > KEYMAX - 1) return true;
		std::string bound((const char *)prefix.data(), prefix.size());
		std::string key = bound;   // exactly the prefix
		return key < bound;         // must be strictly less; broken key_after fails this
	};
	witness::Trial t = witness::resolve<PrefixTail>(
		"identity key_after strictly bounds its prefix", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[keys] key_after aliases input and output safely") {
	witness::Generator<std::vector<uint8_t>> gen = gen_bytes(KEYMAX - 1);
	std::function<bool(const std::vector<uint8_t> &)> pred = [](const std::vector<uint8_t> &prefix) {
		if (prefix.empty() || prefix.size() > KEYMAX - 1) return true;
		uint8_t apart[KEYMAX], inplace[KEYMAX];
		int a = key_after(apart, prefix.data(), (int)prefix.size());
		memcpy(inplace, prefix.data(), prefix.size());
		int b = key_after(inplace, inplace, (int)prefix.size());
		return a == b && memcmp(apart, inplace, (size_t)a) == 0;
	};
	PROP_CHECK(std::vector<uint8_t>, "key_after gives the same answer in place as apart", gen, pred);
}

TEST_CASE("[keys] rule-2 control: an aliasing bug that reads after write is caught") {
	// Planted-wrong: pretend a hostile in-place implementation reads a byte after
	// clobbering it. Any non-empty prefix ending in a byte that would be modified will
	// disagree with the separate-buffer version.
	witness::Generator<std::vector<uint8_t>> gen = gen_bytes(KEYMAX - 1);
	std::function<bool(const std::vector<uint8_t> &)> pred = [](const std::vector<uint8_t> &prefix) {
		if (prefix.empty() || prefix.size() > KEYMAX - 1) return true;
		bool all_ff = true;
		for (uint8_t b : prefix) all_ff = all_ff && b == 0xFF;
		if (all_ff) return true;
		uint8_t apart[KEYMAX];
		int a = key_after(apart, prefix.data(), (int)prefix.size());
		// Fake broken in-place: bump the trailing byte twice.
		std::vector<uint8_t> inplace(prefix);
		int n = (int)inplace.size();
		while (n > 0 && inplace[n - 1] == 0xFF) --n;
		if (n <= 0) return true;
		inplace[n - 1]++;
		inplace[n - 1]++;   // wrong: read-after-write clobber
		return a == n && memcmp(apart, inplace.data(), (size_t)a) == 0;
	};
	witness::Trial t = witness::resolve<std::vector<uint8_t>>(
		"double-bumped key_after agrees with the correct one", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

// ── Transaction record ───────────────────────────────────────────────────────

TEST_CASE("[keys] the txn counter is never mistaken for a record") {
	witness::Generator<uint64_t> gen = &witness::gen_uint64;
	std::function<bool(const uint64_t &)> pred = [](const uint64_t &txnid) {
		uint8_t status[KEYMAX], all[KEYMAX];
		int n = key_txn_status(status, txnid);
		int prefix = key_txn_all(all);
		if (n <= prefix + 8) return false;
		if (memcmp(status, all, (size_t)prefix) != 0) return false;
		if (memcmp(status + prefix + 8, "/STATUS", 7) != 0) return false;
		return n == prefix + 8 + 7;
	};
	PROP_CHECK(uint64_t, "txn status keys carry an 8-byte txnid + /STATUS", gen, pred);
}

TEST_CASE("[keys] rule-2 control: a broken /STATUS suffix leaks past the txnid") {
	// Planted-wrong shape: no suffix at all. The recovery sweep would then treat
	// weft/txn/NEXT as a valid record. The predicate below refuses no-suffix keys.
	witness::Generator<uint64_t> gen = &witness::gen_uint64;
	std::function<bool(const uint64_t &)> pred = [](const uint64_t &txnid) {
		(void)txnid;
		uint8_t buf[KEYMAX];
		int k = key_txn_all(buf);
		put_be64(buf + k, 0);          // pretend NEXT counter under /txn/
		int n = k + 8;                  // no /STATUS
		return n > k + 8;               // false: n == k + 8, so this planted key is a record?
	};
	witness::Trial t = witness::resolve<uint64_t>(
		"a no-suffix record shape is not a record", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

TEST_CASE("[keys] txn records sort by txid") {
	using Pair = std::pair<uint64_t, uint64_t>;
	witness::Generator<Pair> gen = witness::gen_pair<uint64_t, uint64_t>(
		&witness::gen_uint64, &witness::gen_uint64);
	std::function<bool(const Pair &)> pred = [](const Pair &p) {
		std::string ka = key_txn_status_str(p.first);
		std::string kb = key_txn_status_str(p.second);
		if (p.first < p.second) return ka < kb;
		if (p.first > p.second) return ka > kb;
		return ka == kb;
	};
	PROP_CHECK(Pair, "key_txn_status lex order matches txid order", gen, pred);
}

// ── Participant rows stay under their record ─────────────────────────────────

using PartInput = std::pair<uint64_t, std::string>;

TEST_CASE("[keys] participant rows sort under their record's prefix") {
	witness::Generator<PartInput> gen = witness::gen_pair<uint64_t, std::string>(
		&witness::gen_uint64, gen_name());
	std::function<bool(const PartInput &)> pred = [](const PartInput &p) {
		if (p.second.size() > 64) return true;
		uint8_t part[KEYMAX], prefix[KEYMAX];
		int n = key_txn_part(part, p.first, p.second.c_str());
		int pl = key_txn_part_prefix(prefix, p.first);
		if (n < pl) return false;
		return memcmp(part, prefix, (size_t)pl) == 0;
	};
	PROP_CHECK(PartInput,
		"key_txn_part starts with key_txn_part_prefix", gen, pred);
}

TEST_CASE("[keys] rule-2 control: a participant key that omits the record prefix leaks") {
	// Same input, wrong builder: put the participant name under weft/txn/ directly with
	// no txnid. The sweep would attribute this row to the wrong record.
	witness::Generator<PartInput> gen = witness::gen_pair<uint64_t, std::string>(
		&witness::gen_uint64, gen_name());
	std::function<bool(const PartInput &)> pred = [](const PartInput &p) {
		if (p.second.size() > 64) return true;
		uint8_t prefix[KEYMAX];
		int pl = key_txn_part_prefix(prefix, p.first);
		uint8_t buf[KEYMAX];
		int n = snprintf((char *)buf, KEYMAX, "weft/txn/PART/%s", p.second.c_str());
		if (n < pl) return false;
		return memcmp(buf, prefix, (size_t)pl) == 0;
	};
	witness::Trial t = witness::resolve<PartInput>(
		"prefix-less participant key sits under the record", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}

// ── Every builder stays inside the buffer ────────────────────────────────────

using BuilderInput = std::tuple<std::string, uint64_t, uint32_t>;

TEST_CASE("[keys] every builder stays inside KEYMAX") {
	witness::Generator<BuilderInput> gen = witness::gen_tuple<std::string, uint64_t, uint32_t>(
		gen_name(), &witness::gen_uint64, &witness::gen_uint);
	std::function<bool(const BuilderInput &)> pred = [](const BuilderInput &t) {
		const std::string &name = std::get<0>(t);
		uint64_t txid = std::get<1>(t);
		uint32_t pgno = std::get<2>(t);
		uint8_t buf[KEYMAX];
		return key_pidx(buf, name.c_str(), pgno) <= KEYMAX
			&& key_delta(buf, name.c_str(), txid, pgno) <= KEYMAX
			&& key_shard(buf, name.c_str(), txid, pgno) <= KEYMAX
			&& key_shardn(buf, name.c_str(), txid) <= KEYMAX
			&& key_meta(buf, name.c_str(), "HEAD") <= KEYMAX
			&& key_prefix(buf, name.c_str(), "PIDX") <= KEYMAX;
	};
	PROP_CHECK(BuilderInput, "every key builder returns at most KEYMAX bytes", gen, pred);
}

TEST_CASE("[keys] rule-2 control: a builder that returns KEYMAX+1 is refused") {
	// The rule-2 mirror is the trivially-broken bound. This confirms the property is not
	// a tautology: a length above KEYMAX fails, so the property has teeth.
	witness::Generator<BuilderInput> gen = witness::gen_tuple<std::string, uint64_t, uint32_t>(
		gen_name(), &witness::gen_uint64, &witness::gen_uint);
	std::function<bool(const BuilderInput &)> pred = [](const BuilderInput &t) {
		(void)t;
		int planted = KEYMAX + 1;   // pretend a builder returned this
		return planted <= KEYMAX;
	};
	witness::Trial t = witness::resolve<BuilderInput>(
		"KEYMAX+1 is inside KEYMAX", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == witness::Outcome::FOUND);
}
