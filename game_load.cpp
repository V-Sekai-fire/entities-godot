// A constant game workload, for optimising latency.
//
//   game_load [--players N] [--hz H] [--seconds S] [--warmup S] [--prefix NAME]
//             [--bulk-rows R] [--bulk-rows-per-commit K] [--bulk-commits-per-s B]
//
// Players: N threads, each on a database of its own, each committing one row per tick at
// H ticks a second, the shape of a game server saving player state. Bulk: one thread on a
// database of R rows, rewriting a fixed number of rows on a fixed schedule and paying the
// fold whenever one is owed, the shape of background work beside the players. Both run on
// a clock, not as fast as they can: a tick is due at start + i / H whether or not the last
// one finished, and its latency is measured from when it was due. A slow commit then shows
// as the ticks queued behind it, rather than as fewer ticks, which a closed loop would
// hide. The same flags give the same load, so two builds can be compared on one number.
//
// Reports, after the warmup:
//   [TICK]  every player commit, from due to done
//   [LATE]  the share of ticks that finished after the next tick was due
//   [BULK]  each bulk commit, [FOLD] each fold the bulk thread paid
// SPDX-License-Identifier: Apache-2.0

#include <sqlite3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

extern "C" {
int weft_fdb_start(const char *cluster_file);
void weft_fdb_stop(void);
int weft_vfs_register(int make_default);
int weft_compact_due(sqlite3 *db);
}

namespace {

constexpr const char *kVfsName = "weft_fdb";

std::uint64_t now_us() {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch())
            .count());
}

void sleep_until_us(std::uint64_t t) {
    const std::uint64_t n = now_us();
    if (t > n) std::this_thread::sleep_for(std::chrono::microseconds(t - n));
}

void report(const char *label, std::vector<std::uint64_t> v) {
    if (v.empty()) {
        std::printf("[%s] count=0\n", label);
        return;
    }
    std::sort(v.begin(), v.end());
    double sum = 0;
    for (auto x : v) sum += static_cast<double>(x);
    auto at = [&](double p) { return v[static_cast<std::size_t>(p * (v.size() - 1))]; };
    std::printf("[%s] count=%zu avg_us=%.0f p50=%" PRIu64 " p90=%" PRIu64 " p99=%" PRIu64
                " p999=%" PRIu64 " max=%" PRIu64 "\n",
                label, v.size(), sum / v.size(), at(0.5), at(0.9), at(0.99), at(0.999),
                v.back());
}

bool exec(sqlite3 *db, const char *sql) {
    char *err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::fprintf(stderr, "%s -> %s\n", sql, err ? err : "?");
        sqlite3_free(err);
        return false;
    }
    return true;
}

// The store's documented settings: the commit is already atomic, so the journal stays in
// memory, and EXCLUSIVE keeps SQLite's cache across transactions.
sqlite3 *open_table(const std::string &name, int rows) {
    sqlite3 *db = nullptr;
    if (sqlite3_open_v2(name.c_str(), &db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                        kVfsName) != SQLITE_OK) {
        std::fprintf(stderr, "open %s: %s\n", name.c_str(), db ? sqlite3_errmsg(db) : "?");
        return nullptr;
    }
    char sql[320];
    std::snprintf(sql, sizeof sql,
                  "CREATE TABLE IF NOT EXISTS t(k INTEGER PRIMARY KEY, v BLOB);"
                  "BEGIN; WITH RECURSIVE c(i) AS (SELECT 1 UNION ALL SELECT i+1 FROM c"
                  " WHERE i<%d) INSERT OR IGNORE INTO t SELECT i, randomblob(100) FROM c;"
                  " COMMIT;",
                  rows);
    if (!exec(db, "PRAGMA journal_mode=MEMORY") || !exec(db, "PRAGMA locking_mode=EXCLUSIVE")
        || !exec(db, "PRAGMA cache_size=-65536") || !exec(db, sql)) {
        sqlite3_close(db);
        return nullptr;
    }
    return db;
}

struct Config {
    int players = 16;
    double hz = 20;
    double seconds = 30;
    double warmup = 3;
    int bulk_rows = 0;           // 0: no bulk thread
    int bulk_rows_per_commit = 2000;
    double bulk_commits_per_s = 2;
    std::string prefix = "game";
};

std::atomic<bool> g_failed{false};

// One player: a tick due every 1/hz seconds, one row committed per tick.
void player(int id, const Config &c, std::uint64_t start, std::uint64_t end,
            std::uint64_t measure_from, std::vector<std::uint64_t> &lat,
            std::vector<std::uint64_t> &when, std::uint64_t &late) {
    sqlite3 *db = open_table(c.prefix + "-player-" + std::to_string(id) + ".db", 1000);
    if (!db) { g_failed = true; return; }
    sqlite3_stmt *st = nullptr;
    sqlite3_prepare_v2(db, "UPDATE t SET v=randomblob(100) WHERE k=?1", -1, &st, nullptr);
    const double period = 1e6 / c.hz;
    // Players start spread across one period, as real clients would, not all on one edge.
    const std::uint64_t phase = static_cast<std::uint64_t>(period * id / std::max(1, c.players));
    std::uint32_t rng = 0x9e3779b9u ^ static_cast<std::uint32_t>(id);
    for (std::uint64_t i = 0;; ++i) {
        const std::uint64_t due = start + phase + static_cast<std::uint64_t>(period * i);
        if (due >= end) break;
        sleep_until_us(due);
        rng = rng * 1103515245u + 12345u;
        sqlite3_bind_int(st, 1, 1 + static_cast<int>(rng % 1000u));
        if (sqlite3_step(st) != SQLITE_DONE) {
            std::fprintf(stderr, "player %d: %s\n", id, sqlite3_errmsg(db));
            g_failed = true;
            break;
        }
        sqlite3_reset(st);
        const std::uint64_t done = now_us();
        if (due >= measure_from) {
            lat.push_back(done - due);
            when.push_back(due - measure_from);
            if (done - due > static_cast<std::uint64_t>(period)) ++late;
        }
    }
    sqlite3_finalize(st);
    sqlite3_close(db);
}

// The bulk writer: a fixed number of rows rewritten in one commit, on a fixed schedule,
// and the fold paid as soon as it is owed, as a shard thread with no request waiting would.
void bulk(const Config &c, std::uint64_t start, std::uint64_t end, std::uint64_t measure_from,
          std::vector<std::uint64_t> &commits, std::vector<std::uint64_t> &folds) {
    sqlite3 *db = open_table(c.prefix + "-bulk.db", c.bulk_rows);
    if (!db) { g_failed = true; return; }
    sqlite3_stmt *st = nullptr;
    sqlite3_prepare_v2(db, "UPDATE t SET v=randomblob(100) WHERE k>=?1 AND k<?2", -1, &st,
                       nullptr);
    const double period = 1e6 / c.bulk_commits_per_s;
    int next = 1;
    for (std::uint64_t i = 0;; ++i) {
        const std::uint64_t due = start + static_cast<std::uint64_t>(period * i);
        if (due >= end) break;
        sleep_until_us(due);
        const std::uint64_t t0 = now_us();
        sqlite3_bind_int(st, 1, next);
        sqlite3_bind_int(st, 2, next + c.bulk_rows_per_commit);
        if (!exec(db, "BEGIN") || sqlite3_step(st) != SQLITE_DONE || !exec(db, "COMMIT")) {
            std::fprintf(stderr, "bulk: %s\n", sqlite3_errmsg(db));
            g_failed = true;
            break;
        }
        sqlite3_reset(st);
        const std::uint64_t t1 = now_us();
        next += c.bulk_rows_per_commit;
        if (next > c.bulk_rows) next = 1;
        if (weft_compact_due(db) != SQLITE_OK) {
            g_failed = true;
            break;
        }
        const std::uint64_t t2 = now_us();
        if (t0 >= measure_from) {
            commits.push_back(t1 - t0);
            if (t2 - t1 > 1000) folds.push_back(t2 - t1);
        }
    }
    sqlite3_finalize(st);
    sqlite3_close(db);
}

} // namespace

int main(int argc, char **argv) {
    Config c;
    for (int a = 1; a < argc; ++a) {
        const std::string f = argv[a];
        auto next = [&]() -> const char * {
            if (a + 1 >= argc) {
                std::fprintf(stderr, "%s needs a value\n", f.c_str());
                std::exit(2);
            }
            return argv[++a];
        };
        if (f == "--players") c.players = std::atoi(next());
        else if (f == "--hz") c.hz = std::atof(next());
        else if (f == "--seconds") c.seconds = std::atof(next());
        else if (f == "--warmup") c.warmup = std::atof(next());
        else if (f == "--bulk-rows") c.bulk_rows = std::atoi(next());
        else if (f == "--bulk-rows-per-commit") c.bulk_rows_per_commit = std::atoi(next());
        else if (f == "--bulk-commits-per-s") c.bulk_commits_per_s = std::atof(next());
        else if (f == "--prefix") c.prefix = next();
        else { std::fprintf(stderr, "unknown flag %s\n", f.c_str()); return 2; }
    }
    if (weft_fdb_start(std::getenv("WEFT_FDB_CLUSTER_FILE"))) {
        std::fprintf(stderr, "could not start FoundationDB\n");
        return 1;
    }
    weft_vfs_register(0);
    std::printf("== game_load: players=%d hz=%.1f seconds=%.1f warmup=%.1f bulk_rows=%d"
                " bulk_rows_per_commit=%d bulk_commits_per_s=%.2f\n",
                c.players, c.hz, c.seconds, c.warmup, c.bulk_rows, c.bulk_rows_per_commit,
                c.bulk_commits_per_s);

    // Every thread opens and fills its table before the clock starts.
    const std::uint64_t start = now_us() + 2000000;
    const std::uint64_t measure_from = start + static_cast<std::uint64_t>(c.warmup * 1e6);
    const std::uint64_t end = measure_from + static_cast<std::uint64_t>(c.seconds * 1e6);

    std::vector<std::vector<std::uint64_t>> lat(c.players), when(c.players);
    std::vector<std::uint64_t> late(c.players, 0);
    std::vector<std::uint64_t> commits, folds;
    std::vector<std::thread> threads;
    for (int p = 0; p < c.players; ++p)
        threads.emplace_back(player, p, std::cref(c), start, end, measure_from, std::ref(lat[p]),
                             std::ref(when[p]), std::ref(late[p]));
    if (c.bulk_rows > 0)
        threads.emplace_back(bulk, std::cref(c), start, end, measure_from, std::ref(commits),
                             std::ref(folds));
    for (auto &t : threads) t.join();

    std::vector<std::uint64_t> all;
    std::uint64_t all_late = 0;
    for (int p = 0; p < c.players; ++p) {
        all.insert(all.end(), lat[p].begin(), lat[p].end());
        all_late += late[p];
    }
    report("TICK", all);
    // The p99 of each ten-second window: a load that is constant should give windows that
    // agree, and a window that does not is the run's noise, seen rather than averaged in.
    const std::size_t nwin = static_cast<std::size_t>(c.seconds / 10);
    std::vector<std::vector<std::uint64_t>> win(std::max<std::size_t>(nwin, 1));
    for (int p = 0; p < c.players; ++p)
        for (std::size_t i = 0; i < lat[p].size(); ++i)
            win[std::min(win.size() - 1, static_cast<std::size_t>(when[p][i] / 10000000))]
                .push_back(lat[p][i]);
    std::printf("[WINDOW-P99]");
    for (auto &w : win) {
        std::sort(w.begin(), w.end());
        const std::size_t at = w.empty() ? 0 : static_cast<std::size_t>(0.99 * (w.size() - 1));
        std::printf(" %" PRIu64, w.empty() ? 0 : w[at]);
    }
    std::printf("\n");
    std::printf("[LATE] ticks=%zu late=%" PRIu64 " share=%.4f\n", all.size(), all_late,
                all.empty() ? 0.0 : static_cast<double>(all_late) / all.size());
    if (c.bulk_rows > 0) {
        report("BULK", commits);
        report("FOLD", folds);
    }
    weft_fdb_stop();
    return g_failed ? 1 : 0;
}
