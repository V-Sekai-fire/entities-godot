# fabric-store

SQLite runs in a native process with a custom VFS with no local file. The pages live in FoundationDB, so a handoff copies nothing: another machine opens the
same database and reads pages.

## What callers see

Callers reach the store over iceoryx2, one shard per service, one thread per shard. Nothing outside the store process opens a SQLite handle: `store_driver.cpp` is the reference caller and it deliberately does not link `weft_fdb_vfs` or include `sqlite3.h`. `store.cpp:1-7` states the same rule in prose ("a caller never opens a database"). SQL, the VFS, and the `?vfs=weft_fdb` URL are internal.

The loadable-extension form of the VFS (`fdb_vfs_ext.c`, packaged via `V-Sekai-fire/sqlite-jdbc`) exposes SQL over JDBC. **That path is a benchmarking affordance for the `entities-vsk-database-roundtable` YCSB round, not a supported product API.** Shipping code goes through iceoryx2.

## Licence

Licensed under either of

* Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE))
* MIT License ([LICENSE-MIT](LICENSE-MIT))

at your option.

`SPDX-License-Identifier: Apache-2.0 OR MIT`

### Scope

These terms cover this repository's own code. They do not relicense vendored code under
`thirdparty/`, which keeps the terms it arrived with. `thirdparty/harness` is a subtree of the
harness repository and is governed there. It vendors code in turn, and
`thirdparty/harness/thirdparty/generate_stubs/LICENSE.chromium` is the licence that applies to
that part.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion
in this work by you shall be dual licensed as above, without any additional terms or
conditions.
