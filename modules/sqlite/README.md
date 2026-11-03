# Godot VSK SQLite

This is a [SQLite](https://github.com/V-Sekai/godot-vsk-sqlite) wrapper for the Godot Engine. The database pages live in FoundationDB, not in a file: `SQLite.open("user://save.db")` opens the database `save.db`.

## FoundationDB

- The `weft_fdb` VFS comes from [V-Sekai-fire/datasource-store](https://github.com/V-Sekai-fire/datasource-store), a subtree in `thirdparty/datasource-store`. Update it with `git subtree pull --squash`.
- The module builds for x86_64 Linux, x86_64 Windows with MinGW (`use_mingw=yes`) and arm64 macOS. Other platforms and MSVC/clang-cl builds leave it out.
- CI takes the client from a release of [datasource-foundationdb](https://github.com/V-Sekai-fire/datasource-foundationdb/releases) through that repository's `fdb-client` action: headers from the MSI (the pkg on macOS), `libfdb_c` from the Linux and macOS assets, `fdb_c.lib`/`fdb_c.dll` from the MSI on Windows. A Windows binary needs `fdb_c.dll` next to it or on `PATH`.
- The build needs the FoundationDB client (`foundationdb-clients`, 7.3). Pass `fdb_include=<dir>` (holds `fdb_c.h`) and `fdb_libpath=<dir>` (holds `libfdb_c`) when they are not in `/usr/include/foundationdb` and `/usr/lib`.
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
