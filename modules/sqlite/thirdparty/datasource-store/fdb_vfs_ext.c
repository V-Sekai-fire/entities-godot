// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// The weft_fdb VFS as a loadable SQLite extension, for a host that brings its own SQLite
// (exqlite inside the BEAM) and so cannot link fdb_vfs.c beside it. sqlite3ext.h comes
// first, so every SQLite call in fdb_vfs.c goes through the host's API table.
//
//   .load ./weftfdb                     entry point sqlite3_weftfdb_init, from the file name
//   file:<name>?vfs=weft_fdb            then open with the two pragmas fdb_vfs.c requires
#define _POSIX_C_SOURCE 200809L
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1
#include "fdb_vfs.c"

static pthread_once_t g_ext_once = PTHREAD_ONCE_INIT;
static int g_ext_fdb_err;
static int g_ext_rc;

// The FoundationDB network starts once per process, however many connections load us.
static void ext_start(void) {
	g_ext_fdb_err = weft_fdb_start(getenv("WEFT_FDB_CLUSTER_FILE"));
	g_ext_rc = g_ext_fdb_err ? SQLITE_CANTOPEN : weft_vfs_register(0);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_weftfdb_init(sqlite3 *db, char **err, const sqlite3_api_routines *api) {
	SQLITE_EXTENSION_INIT2(api);
	(void)db;
	pthread_once(&g_ext_once, ext_start);
	if (g_ext_rc != SQLITE_OK) {
		const char *cf = getenv("WEFT_FDB_CLUSTER_FILE");
		*err = sqlite3_mprintf("weft_fdb did not start: %s (WEFT_FDB_CLUSTER_FILE=%s)",
			g_ext_fdb_err ? fdb_get_error(g_ext_fdb_err) : "VFS registration failed",
			cf ? cf : "unset, so FoundationDB's default");
		return SQLITE_ERROR;
	}
	return SQLITE_OK_LOAD_PERMANENTLY;
}
