# Godot VSK SQLite

This is a [SQLite](https://github.com/V-Sekai/godot-vsk-sqlite) wrapper for the Godot Engine. The database pages live in FoundationDB, not in a file: `SQLite.open("user://save.db")` opens the database `save.db`.

## FoundationDB

- The `weft_fdb` VFS comes from [V-Sekai-fire/datasource-store](https://github.com/V-Sekai-fire/datasource-store), a subtree in `thirdparty/datasource-store`. Update it with `git subtree pull --squash`.
- The module builds on every platform and architecture except MSVC and clang-cl (the VFS needs POSIX threads). The FoundationDB client is not linked: `fdb/fdb_c.sigs` lists the entry points the VFS calls, `thirdparty/generate_stubs` generates a dlsym dispatch table from it (the pattern of `modules/native_media`), and `fdb/fdb_client.cpp` opens `libfdb_c` at run time. The build needs no client package and no `fdb_c.h`.
- At run time put the client library (`libfdb_c.so`, `libfdb_c.dylib` or `fdb_c.dll`, FoundationDB 7.3, from a release of [datasource-foundationdb](https://github.com/V-Sekai-fire/datasource-foundationdb/releases) or the foundationdb-clients package) beside the binary or on the loader's search path. Without it `open` returns an error and `open_in_memory` and `open_buffered` still work.
- At run time the client reads the cluster file named by `WEFT_FDB_CLUSTER_FILE`, or FoundationDB's default.
- `open` sets `PRAGMA journal_mode=MEMORY` and `PRAGMA locking_mode=EXCLUSIVE`, which the VFS requires: one handle is the only writer of its database.
- `open_in_memory` and `open_buffered` (used for `res://`) still use SQLite's own memory storage.

## Installation

1. Clone the repository using git.

   ```
   git clone https://github.com/V-Sekai/godot-vsk-sqlite.git sqlite
   ```

## Example Usage

- [SQL Queries](https://github.com/V-Sekai/godot-vsk-sqlite/blob/master/demo/SQLite/sql_queries.gd)
- [Game Highscore](https://github.com/V-Sekai/godot-vsk-sqlite/blob/master/demo/SQLite/game_highscore.gd)
- [Item Libraries](https://github.com/V-Sekai/godot-vsk-sqlite/blob/master/demo/SQLite/item_database.gd)

## Credits

This engine module for Godot is based on `gdsqlite-native` by Khairul Hidayat in 2017.
