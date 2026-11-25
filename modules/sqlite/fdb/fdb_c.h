// The slice of the FoundationDB C API (version 730) that modules/sqlite uses, declared here so the
// build needs neither foundationdb-clients nor its fdb_c.h. The definitions come from libfdb_c at
// run time through the stubs generated from fdb_c.sigs. Written to match the public C API's ABI.
#pragma once

#include <stdint.h>

#ifndef FDB_API_VERSION
#define FDB_API_VERSION 730
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int fdb_error_t;
typedef int fdb_bool_t;

typedef struct FDB_future FDBFuture;
typedef struct FDB_database FDBDatabase;
typedef struct FDB_transaction FDBTransaction;

#pragma pack(push, 4)
typedef struct keyvalue {
	const uint8_t *key;
	int key_length;
	const uint8_t *value;
	int value_length;
} FDBKeyValue;
#pragma pack(pop)

typedef enum {
	FDB_STREAMING_MODE_WANT_ALL = -2,
	FDB_STREAMING_MODE_ITERATOR = -1,
	FDB_STREAMING_MODE_EXACT = 0,
	FDB_STREAMING_MODE_SMALL = 1,
	FDB_STREAMING_MODE_MEDIUM = 2,
	FDB_STREAMING_MODE_LARGE = 3,
	FDB_STREAMING_MODE_SERIAL = 4
} FDBStreamingMode;

#define FDB_KEYSEL_LAST_LESS_THAN(k, l) k, l, 0, 0
#define FDB_KEYSEL_LAST_LESS_OR_EQUAL(k, l) k, l, 1, 0
#define FDB_KEYSEL_FIRST_GREATER_THAN(k, l) k, l, 1, 1
#define FDB_KEYSEL_FIRST_GREATER_OR_EQUAL(k, l) k, l, 0, 1

#define fdb_select_api_version(v) fdb_select_api_version_impl(v, FDB_API_VERSION)

// Prototypes: fdb_c.sigs holds the same lines, and the generated stubs define them.
fdb_error_t fdb_select_api_version_impl(int runtime_version, int header_version);
const char *fdb_get_error(fdb_error_t code);
fdb_error_t fdb_setup_network(void);
fdb_error_t fdb_run_network(void);
fdb_error_t fdb_stop_network(void);
fdb_error_t fdb_create_database(const char *cluster_file_path, FDBDatabase **out_database);
void fdb_database_destroy(FDBDatabase *d);
fdb_error_t fdb_database_create_transaction(FDBDatabase *d, FDBTransaction **out_transaction);
void fdb_transaction_destroy(FDBTransaction *tr);
FDBFuture *fdb_transaction_get(FDBTransaction *tr, const uint8_t *key_name, int key_name_length, fdb_bool_t snapshot);
FDBFuture *fdb_transaction_get_range(FDBTransaction *tr, const uint8_t *begin_key_name, int begin_key_name_length, fdb_bool_t begin_or_equal, int begin_offset, const uint8_t *end_key_name, int end_key_name_length, fdb_bool_t end_or_equal, int end_offset, int limit, int target_bytes, FDBStreamingMode mode, int iteration, fdb_bool_t snapshot, fdb_bool_t reverse);
void fdb_transaction_set(FDBTransaction *tr, const uint8_t *key_name, int key_name_length, const uint8_t *value, int value_length);
void fdb_transaction_clear_range(FDBTransaction *tr, const uint8_t *begin_key_name, int begin_key_name_length, const uint8_t *end_key_name, int end_key_name_length);
FDBFuture *fdb_transaction_commit(FDBTransaction *tr);
FDBFuture *fdb_transaction_on_error(FDBTransaction *tr, fdb_error_t error);
fdb_error_t fdb_future_block_until_ready(FDBFuture *f);
fdb_error_t fdb_future_get_error(FDBFuture *f);
fdb_error_t fdb_future_get_value(FDBFuture *f, fdb_bool_t *out_present, const uint8_t **out_value, int *out_value_length);
fdb_error_t fdb_future_get_keyvalue_array(FDBFuture *f, const FDBKeyValue **out_kv, int *out_count, fdb_bool_t *out_more);
void fdb_future_destroy(FDBFuture *f);

// Opens libfdb_c (fdb_client.cpp). Returns 0 when the client is loaded, nonzero when it is not.
int weft_fdb_client_load(void);

#ifdef __cplusplus
}
#endif
