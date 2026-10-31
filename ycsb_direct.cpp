// YCSB workload F driver: N workers, `usertable`, 50 % READ / 50 % RMW.
//
//   ycsb_direct <recordcount> <opcount> [workers] [batch]
//
// SPDX-License-Identifier: Apache-2.0
#include <sqlite3.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
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
int weft_compact_due(sqlite3 *db);
int weft_vfs_register(int make_default);
}

namespace {

constexpr const char *kVfsName = "weft_fdb";
constexpr const char *kTableName = "usertable";

std::uint64_t now_us() {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch())
            .count());
}

std::uint64_t percentile(std::vector<std::uint64_t>& v, double p) {
    if (v.empty()) return 0;
    const std::size_t idx = static_cast<std::size_t>((p / 100.0) * (v.size() - 1));
    return v[idx];
}

void report(const char* label, std::vector<std::uint64_t>& lat) {
    if (lat.empty()) { std::printf("[%s] no samples\n", label); return; }
    std::sort(lat.begin(), lat.end());
    double sum = 0.0;
    for (auto x : lat) sum += static_cast<double>(x);
    std::printf("[%s] count=%zu avg_us=%.0f p50=%" PRIu64 " p95=%" PRIu64
                " p99=%" PRIu64 " max=%" PRIu64 "\n",
                label, lat.size(), sum / lat.size(),
                percentile(lat, 50.0), percentile(lat, 95.0),
                percentile(lat, 99.0), lat.back());
}

struct Options {
    bool sorted_load = false;
    bool nomutex = false;
    bool verify = false;
};

// Every worker loads, waits here, and only then starts the timed run, so the run's wall
// time is one interval shared by all workers rather than a sum of staggered ones.
class Barrier {
public:
    explicit Barrier(unsigned n) : left_(n) {}
    void arrive_and_wait() {
        std::unique_lock<std::mutex> lock(m_);
        if (--left_ == 0) { cv_.notify_all(); return; }
        cv_.wait(lock, [this] { return left_ == 0; });
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    unsigned left_;
};

struct WorkerResult {
    std::vector<std::uint64_t> load_lat;
    std::vector<std::uint64_t> read_lat;
    std::vector<std::uint64_t> rmw_lat;
    std::vector<std::uint64_t> commit_lat;
    std::uint64_t close_us = 0;
    std::uint64_t run_start_us = 0;
    std::uint64_t run_end_us = 0;
    bool ok = true;
};

std::string g_prefix = "ycsb";

std::string db_filename_for(unsigned worker_id) {
    return g_prefix + "-" + std::to_string(worker_id) + ".db";
}

sqlite3 *open_db(const std::string &filename, const char *vfs, const Options &opt) {
    sqlite3 *db = nullptr;
    // One connection per worker thread, never shared, so its mutex guards nothing.
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
                      | (opt.nomutex ? SQLITE_OPEN_NOMUTEX : 0);
    if (sqlite3_open_v2(filename.c_str(), &db, flags, vfs) != SQLITE_OK) {
        std::fprintf(stderr, "open %s: %s\n", filename.c_str(),
                     db ? sqlite3_errmsg(db) : "(no handle)");
        if (db) sqlite3_close(db);
        return nullptr;
    }
    sqlite3_exec(db, "PRAGMA journal_mode=MEMORY", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA locking_mode=EXCLUSIVE", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA cache_size=-4194304", nullptr, nullptr, nullptr);
    return db;
}

int create_usertable(sqlite3 *db) {
    const char *ddl =
        "CREATE TABLE IF NOT EXISTS usertable (YCSB_KEY VARCHAR(255) PRIMARY KEY, "
        "FIELD0 TEXT, FIELD1 TEXT, FIELD2 TEXT, FIELD3 TEXT, FIELD4 TEXT, "
        "FIELD5 TEXT, FIELD6 TEXT, FIELD7 TEXT, FIELD8 TEXT, FIELD9 TEXT)";
    return sqlite3_exec(db, ddl, nullptr, nullptr, nullptr);
}

bool load_phase(sqlite3 *db, std::uint64_t recordcount, std::uint64_t batch,
                const Options &opt, WorkerResult &out) {
    sqlite3_stmt *ins = nullptr;
    if (sqlite3_prepare_v3(db,
        "INSERT INTO usertable VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11)",
        -1, SQLITE_PREPARE_PERSISTENT, &ins, nullptr) != SQLITE_OK) return false;

    const std::string value(100, 'x');
    // The same keys, inserted in the order the B-tree sorts them, so each commit appends
    // pages instead of rewriting pages an earlier commit already sent.
    std::vector<std::string> keys;
    keys.reserve(recordcount);
    for (std::uint64_t i = 0; i < recordcount; ++i) {
        char key[32]; std::snprintf(key, sizeof key, "user%" PRIu64, i);
        keys.emplace_back(key);
    }
    if (opt.sorted_load) std::sort(keys.begin(), keys.end());
    out.load_lat.reserve(recordcount / batch + 1);
    for (std::uint64_t base = 0; base < recordcount; base += batch) {
        const std::uint64_t hi = std::min(base + batch, recordcount);
        const std::uint64_t t = now_us();
        sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
        for (std::uint64_t i = base; i < hi; ++i) {
            const std::string &key = keys[i];
            sqlite3_bind_text(ins, 1, key.data(), (int)key.size(), SQLITE_STATIC);
            for (int f = 0; f < 10; ++f) {
                sqlite3_bind_text(ins, 2 + f, value.data(), (int)value.size(), SQLITE_STATIC);
            }
            if (sqlite3_step(ins) != SQLITE_DONE) {
                std::fprintf(stderr, "load step: %s\n", sqlite3_errmsg(db));
                sqlite3_finalize(ins); return false;
            }
            sqlite3_reset(ins);
        }
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
        out.load_lat.push_back(now_us() - t);
    }
    sqlite3_finalize(ins);
    return true;
}

bool run_phase(sqlite3 *db, std::uint32_t seed, std::uint64_t recordcount,
               std::uint64_t opcount, std::uint64_t batch, WorkerResult &out) {
    sqlite3_stmt *sel = nullptr, *upd = nullptr;
    if (sqlite3_prepare_v3(db, "SELECT * FROM usertable WHERE YCSB_KEY=?1",
                           -1, SQLITE_PREPARE_PERSISTENT, &sel, nullptr) != SQLITE_OK
        || sqlite3_prepare_v3(db, "UPDATE usertable SET FIELD0=?1 WHERE YCSB_KEY=?2",
                              -1, SQLITE_PREPARE_PERSISTENT, &upd, nullptr) != SQLITE_OK) {
        if (sel) sqlite3_finalize(sel);
        if (upd) sqlite3_finalize(upd);
        return false;
    }

    out.read_lat.reserve(opcount / 2 + 1);
    out.rmw_lat.reserve(opcount / 2 + 1);
    std::uint32_t rng = seed;
    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
    std::uint64_t in_txn = 0;
    for (std::uint64_t i = 0; i < opcount; ++i) {
        rng = rng * 1103515245u + 12345u;
        const std::uint64_t k = static_cast<std::uint64_t>(rng) % recordcount;
        char key[32]; std::snprintf(key, sizeof key, "user%" PRIu64, k);
        if ((i & 1u) == 0u) {
            const std::uint64_t t = now_us();
            sqlite3_bind_text(sel, 1, key, -1, SQLITE_TRANSIENT);
            while (sqlite3_step(sel) == SQLITE_ROW) { /* drop rows */ }
            sqlite3_reset(sel);
            out.read_lat.push_back(now_us() - t);
        } else {
            const std::uint64_t t = now_us();
            char v[32]; std::snprintf(v, sizeof v, "v%" PRIu64, i);
            sqlite3_bind_text(upd, 1, v, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(upd, 2, key, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(upd) != SQLITE_DONE) {
                std::fprintf(stderr, "update: %s\n", sqlite3_errmsg(db));
                sqlite3_finalize(sel); sqlite3_finalize(upd); return false;
            }
            sqlite3_reset(upd);
            out.rmw_lat.push_back(now_us() - t);
            if (++in_txn >= batch) {
                const std::uint64_t c = now_us();
                sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
                out.commit_lat.push_back(now_us() - c);
                sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
                in_txn = 0;
            }
        }
    }
    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(sel);
    sqlite3_finalize(upd);
    return true;
}

// FNV-1a over every column of every row in key order, so a fold that changed any byte of
// any row changes the answer.
bool table_hash(sqlite3 *db, std::uint64_t &h, std::uint64_t &rows) {
    sqlite3_stmt *st = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT * FROM usertable ORDER BY YCSB_KEY", -1, &st, nullptr)
        != SQLITE_OK) return false;
    h = 1469598103934665603ULL;
    rows = 0;
    while (sqlite3_step(st) == SQLITE_ROW) {
        ++rows;
        for (int col = 0; col < sqlite3_column_count(st); ++col) {
            const unsigned char *t = sqlite3_column_text(st, col);
            const int n = sqlite3_column_bytes(st, col);
            for (int i = 0; i < n; ++i) { h ^= t[i]; h *= 1099511628211ULL; }
            h ^= 0xff; h *= 1099511628211ULL;
        }
    }
    sqlite3_finalize(st);
    return true;
}

// Close the database, which pays any fold owed, then open it again and require the same
// rows and a clean integrity check.
bool close_and_check(sqlite3 *&db, const std::string &filename, const Options &opt,
                     unsigned worker_id, int cycle) {
    std::uint64_t before = 0, rows = 0, after = 0, rows_after = 0;
    if (!table_hash(db, before, rows)) return false;
    sqlite3_close(db);
    db = open_db(filename, kVfsName, opt);
    if (!db) return false;
    sqlite3_stmt *st = nullptr;
    bool ok = sqlite3_prepare_v2(db, "PRAGMA integrity_check", -1, &st, nullptr) == SQLITE_OK
              && sqlite3_step(st) == SQLITE_ROW
              && std::strcmp((const char *)sqlite3_column_text(st, 0), "ok") == 0;
    sqlite3_finalize(st);
    ok = ok && table_hash(db, after, rows_after) && after == before && rows_after == rows;
    std::printf("[VERIFY] worker=%u cycle=%d rows=%" PRIu64 " %s\n", worker_id, cycle, rows,
                ok ? "ok" : "MISMATCH");
    return ok;
}

void run_worker(unsigned worker_id, std::uint64_t recordcount, std::uint64_t opcount,
                std::uint64_t batch, const Options &opt, Barrier &loaded, WorkerResult &out) {
    const std::string filename = db_filename_for(worker_id);
    sqlite3 *db = open_db(filename, kVfsName, opt);
    bool ok = db != nullptr;
    if (ok && create_usertable(db) != SQLITE_OK) {
        std::fprintf(stderr, "worker %u create: %s\n", worker_id, sqlite3_errmsg(db));
        ok = false;
    }
    if (ok) ok = load_phase(db, recordcount, batch, opt, out);
    // Every worker arrives, failed or not, so no worker waits forever on one that stopped.
    loaded.arrive_and_wait();
    const std::uint32_t seed = 0x9e3779b9u ^ worker_id;
    if (ok) {
        out.run_start_us = now_us();
        ok = run_phase(db, seed, recordcount, opcount, batch, out);
        out.run_end_us = now_us();
    }
    // Two cycles: the first fold reads only the log, the second a shard and a log.
    if (ok && opt.verify) {
        WorkerResult scratch;
        // The second cycle folds in the middle and goes on writing on the same handle,
        // so a delta taken against a base the fold deleted is reached.
        ok = close_and_check(db, filename, opt, worker_id, 1)
             && run_phase(db, seed + 1, recordcount, opcount, batch, scratch)
             && weft_compact_due(db) == SQLITE_OK
             && run_phase(db, seed + 2, recordcount, opcount, batch, scratch)
             && close_and_check(db, filename, opt, worker_id, 2);
    }
    out.ok = ok;
    const std::uint64_t c = now_us();
    if (db) sqlite3_close(db);
    out.close_us = now_us() - c;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: ycsb_direct <recordcount> <opcount> [workers] [batch]"
                             " [--sorted-load] [--nomutex] [--verify] [--prefix NAME]\n"
                             "\n"
                             "  YCSB workload F: 50%% READ + 50%% RMW on `usertable`.\n"
                             "  One SQLite database file per worker (roundtable shape).\n");
        return 2;
    }
    const std::uint64_t recordcount = std::strtoull(argv[1], nullptr, 10);
    const std::uint64_t opcount = std::strtoull(argv[2], nullptr, 10);
    const std::uint32_t workers = (argc > 3) ? (std::uint32_t)std::atoi(argv[3]) : 1u;
    const std::uint64_t batch = (argc > 4) ? std::strtoull(argv[4], nullptr, 10) : 1ULL;
    Options opt;
    for (int a = 5; a < argc; ++a) {
        const std::string flag = argv[a];
        if (flag == "--sorted-load") opt.sorted_load = true;
        else if (flag == "--nomutex") opt.nomutex = true;
        else if (flag == "--verify") opt.verify = true;
        else if (flag == "--prefix" && a + 1 < argc) g_prefix = argv[++a];
        else { std::fprintf(stderr, "unknown flag %s\n", argv[a]); return 2; }
    }
    // Before anything initialises SQLite: no global memory statistics, which cost a mutex
    // on every allocation.
    if (opt.nomutex) sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0);

    if (weft_fdb_start(std::getenv("WEFT_FDB_CLUSTER_FILE"))) {
        std::fprintf(stderr, "FoundationDB did not start\n");
        return 1;
    }
    weft_vfs_register(0);

    std::printf("== ycsb_direct: workers=%u recordcount=%" PRIu64 " opcount=%" PRIu64
                " batch=%" PRIu64 " sorted_load=%d nomutex=%d\n",
                workers, recordcount, opcount, batch, opt.sorted_load, opt.nomutex);

    std::vector<WorkerResult> results(workers);
    Barrier loaded(workers);
    std::vector<std::thread> threads;
    threads.reserve(workers);
    const std::uint64_t wall_start = now_us();
    for (std::uint32_t w = 0; w < workers; ++w) {
        threads.emplace_back(run_worker, w, recordcount, opcount, batch, std::cref(opt),
                             std::ref(loaded), std::ref(results[w]));
    }
    for (auto& t : threads) t.join();
    const std::uint64_t wall_us = now_us() - wall_start;

    std::vector<std::uint64_t> all_read, all_rmw, all_load, all_commit, all_close;
    std::uint64_t total_ops = 0;
    std::uint64_t run_start = UINT64_MAX, run_end = 0;
    for (auto& r : results) {
        if (!r.ok) { weft_fdb_stop(); return 1; }
        run_start = std::min(run_start, r.run_start_us);
        run_end = std::max(run_end, r.run_end_us);
        total_ops += r.read_lat.size() + r.rmw_lat.size();
        all_read.insert(all_read.end(), r.read_lat.begin(), r.read_lat.end());
        all_rmw.insert(all_rmw.end(), r.rmw_lat.begin(), r.rmw_lat.end());
        all_load.insert(all_load.end(), r.load_lat.begin(), r.load_lat.end());
        all_commit.insert(all_commit.end(), r.commit_lat.begin(), r.commit_lat.end());
        all_close.push_back(r.close_us);
    }
    const double wall_sec = wall_us / 1e6;
    std::printf("[OVERALL] wall_s=%.3f workers=%u throughput=%.1f ops/sec\n",
                wall_sec, workers, static_cast<double>(total_ops) / wall_sec);
    // The run phase alone, as YCSB's own `run` step reports it: the load is not counted.
    const double run_sec = (run_end - run_start) / 1e6;
    std::printf("[RUN] wall_s=%.3f workers=%u throughput=%.1f ops/sec\n",
                run_sec, workers, static_cast<double>(total_ops) / run_sec);
    report("READ", all_read);
    report("RMW", all_rmw);
    report("LOAD", all_load);
    report("COMMIT", all_commit);
    report("CLOSE", all_close);

    weft_fdb_stop();
    return 0;
}
