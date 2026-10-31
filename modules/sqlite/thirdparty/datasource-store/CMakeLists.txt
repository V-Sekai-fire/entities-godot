# The store: SQLite with a VFS whose pages live in FoundationDB.
# See README.md.
#
# C for the VFS and the programs that exercise it, C++ for the store, because the harness
# and the iceoryx2 dispatch table it generates are C++.
cmake_minimum_required(VERSION 3.22)
project(weft_store C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# clangd reads compile_commands.json for the real include paths and flags. Without it
# clangd guesses, and every FoundationDB and SQLite header looks missing.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_path(FDB_INCLUDE_DIR foundationdb/fdb_c.h REQUIRED)
find_library(FDB_LIBRARY NAMES fdb_c REQUIRED)
find_path(SQLITE_INCLUDE_DIR sqlite3.h REQUIRED)
find_library(SQLITE_LIBRARY NAMES sqlite3 REQUIRED)

add_library(weft_fdb_vfs STATIC fdb_vfs.c)
# Position-independent, so the library can be linked into a shared object.
set_property(TARGET weft_fdb_vfs PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(weft_fdb_vfs PUBLIC
  ${FDB_INCLUDE_DIR}/foundationdb ${SQLITE_INCLUDE_DIR})
target_compile_definitions(weft_fdb_vfs PUBLIC FDB_API_VERSION=730)
target_link_libraries(weft_fdb_vfs PUBLIC ${FDB_LIBRARY} ${SQLITE_LIBRARY} Threads::Threads)

# The VFS as a loadable extension for a host that brings its own SQLite (exqlite in the BEAM).
# fdb_vfs_ext.c compiles fdb_vfs.c against sqlite3ext.h, so the module links no SQLite.
add_library(weftfdb MODULE fdb_vfs_ext.c)
set_target_properties(weftfdb PROPERTIES PREFIX "" POSITION_INDEPENDENT_CODE ON)
if(APPLE)
  set_target_properties(weftfdb PROPERTIES SUFFIX ".dylib")
endif()
target_include_directories(weftfdb PRIVATE
  ${FDB_INCLUDE_DIR}/foundationdb ${SQLITE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(weftfdb PRIVATE FDB_API_VERSION=730)
target_link_libraries(weftfdb PRIVATE ${FDB_LIBRARY} Threads::Threads)

# The programs that assert something in-process. These are the default build: each opens a
# database, checks a property and exits, so a plain `cmake --build` produces exactly the set
# a developer can run without arranging anything first.
foreach(prog prove_big_commit integrity)
  add_executable(${prog} ${prog}.c)
  target_link_libraries(${prog} PRIVATE weft_fdb_vfs)
endforeach()

# The fault fixtures, and why they are EXCLUDE_FROM_ALL.
#
# None of these can become an in-process test case, and the reason is the property rather
# than the style. `prove_handoff` needs a *second process*, because what it asserts is that a
# different process with no local file opens the same database — one process cannot witness
# that. `prove_crash` and `prove_parallel_commit` SIGKILL themselves at a chosen commit, so
# linking them into a test runner would kill the runner. Dying is the test.
#
# So they stay separate executables that something else spawns. Keeping them out of the
# default target is the honest encoding of that: they are not built for a developer who wants
# the store, they are built by the stage that spawns them.
#
#   cmake --build BUILD --target fixtures
add_custom_target(fixtures)
foreach(prog prove_handoff prove_crash prove_parallel_commit prove_fence_read prove_clock)
  add_executable(${prog} EXCLUDE_FROM_ALL ${prog}.c)
  target_link_libraries(${prog} PRIVATE weft_fdb_vfs)
  add_dependencies(fixtures ${prog})
endforeach()

# The benchmarks. Out of the default build for a different reason: they assert nothing. A
# benchmark reports a number, and a number with no assertion cannot fail a commit stage, so
# building them by default only slows down the build that does.
#
# `tpcc` is not in the loop because it is the only program here that needs threads of its
# own: a terminal is a thread, and the number of them is the axis the benchmark sweeps.
#
#   cmake --build BUILD --target benchmarks
add_custom_target(benchmarks)
add_executable(bench_vfs EXCLUDE_FROM_ALL bench_vfs.c)
target_link_libraries(bench_vfs PRIVATE weft_fdb_vfs)
add_executable(tpcc EXCLUDE_FROM_ALL tpcc.c)
target_link_libraries(tpcc PRIVATE weft_fdb_vfs Threads::Threads m)
# `game_load` holds a constant game-shaped load, players on a tick and bulk work on a
# schedule, and reports the players' commit latency: the number to optimise for VR.
add_executable(game_load EXCLUDE_FROM_ALL game_load.cpp)
target_link_libraries(game_load PRIVATE weft_fdb_vfs Threads::Threads)
target_compile_options(game_load PRIVATE -Wall -Wextra)
add_dependencies(benchmarks bench_vfs tpcc game_load)

# `sqlrun` is a tool, not a test: it runs whatever SQL arrives on stdin and reports rows and
# elapsed time. It asserts nothing, so it is out of the default build for the same reason the
# benchmarks are -- and it is separate from them because a benchmark reports a fixed number
# and this reports whatever was asked.
#
#   cmake --build BUILD --target tools
add_custom_target(tools)
add_executable(sqlrun EXCLUDE_FROM_ALL sqlrun.c)
target_link_libraries(sqlrun PRIVATE weft_fdb_vfs)

# The store helper consumers drive over a line protocol. Built here so the
# programs that link the VFS and the programs that speak to it share one source.
add_executable(weft_sql EXCLUDE_FROM_ALL weft_sql.c)
target_link_libraries(weft_sql PRIVATE weft_fdb_vfs)
add_dependencies(tools sqlrun)

# Rung 11 — direct-linked YCSB workload F. Each worker opens sqlite3* against
# the FDB VFS in-process; no daemon, no iceoryx2 message dispatch. Measured
# 7.2× the throughput of the old iceoryx2 path (12,501 → 90,391 ops/s at
# 8 workers × batch 1000) — the daemon was the wall.
add_executable(ycsb_direct ycsb_direct.cpp)
target_link_libraries(ycsb_direct PRIVATE weft_fdb_vfs Threads::Threads)
target_compile_options(ycsb_direct PRIVATE -Wall -Wextra)
