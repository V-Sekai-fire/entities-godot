#pragma once

// Types for the stubs generated from fdb_c.sigs.
#include "modules/sqlite/fdb/fdb_c.h" // IWYU pragma: keep

// generate_stubs.py emits this on every dispatched call to opt out of Chromium's CFI check.
#define DISABLE_CFI_ICALL
