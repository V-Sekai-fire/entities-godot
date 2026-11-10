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
find_path(SQLITE_INCLUDE_DIR sqlite3.h REQUIRED)
find_library(SQLITE_LIBRARY NAMES sqlite3 REQUIRED)

if(APPLE)
  # On this machine `xcode-select --show-sdk-path` is unavailable, so CMake leaves
  # CMAKE_OSX_SYSROOT empty and `-framework` / SDK-resolved symbols fail to link.
  # Locate the macOS SDK root from the active developer directory (CLT as fallback)
  # and point the build at it.
  execute_process(COMMAND xcode-select --print-path
                 OUTPUT_VARIABLE XCODE_DEVELOPER_DIR
                 ERROR_VARIABLE XCODE_SEL_ERR
                 )
  set(MACOSX_SDK "")
  if(XCODE_DEVELOPER_DIR)
    set(MACOSX_SDK "${XCODE_DEVELOPER_DIR}/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
  endif()
  if(NOT EXISTS "${MACOSX_SDK}/System/Library/Frameworks/CoreFoundation.framework")
    set(MACOSX_SDK "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
  endif()
  if(EXISTS "${MACOSX_SDK}/System/Library/Frameworks/CoreFoundation.framework")
    if(NOT CMAKE_OSX_SYSROOT)
      message(STATUS "Using macOS SDK: ${MACOSX_SDK}")
      set(CMAKE_OSX_SYSROOT "${MACOSX_SDK}")
    endif()
  endif()
endif()

# FoundationDB C client: static, from the subtrees fork
# (modules/sqlite/thirdparty/foundationdb), configured and built by that fork's own
# CMake (FDB_BUILD_FDB_C_STATIC=ON, python tools, no bindings/docs). It is a standalone
# CMake project, so we consume its build tree directly.
set(FDB_SRC_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/../foundationdb)
set(FDB_BUILD_DIR ${FDB_SRC_DIR}/build)
set(FDB_LIB_DIR   ${FDB_BUILD_DIR}/lib)
set(FDB_BOOST_LIB ${FDB_SRC_DIR}/build/boost_install/lib)
# Source + generated C headers live in two directories.
set(FDB_INCLIB    ${FDB_SRC_DIR}/bindings/c/foundationdb)      # fdb_c.h, fdb_c_types.h, fdb_c_internal.h
set(FDB_GEN_INCLIB ${FDB_BUILD_DIR}/bindings/c/foundationdb)    # fdb_c_options.g.h, fdb_c_apiversion.g.h
if(APPLE)
  # The fork statically links its own copy of OpenSSL; link the same one it used.
  set(FDB_OPENSSL_LIB_DIR /opt/homebrew/opt/openssl@3
        CACHE STRING "OpenSSL prefix the FoundationDB client statically linked")
  # The C client chain needs the IOKit/CoreFoundation frameworks on macOS (flow's
  # platform code references _kIOMasterPortDefault / _kCFAllocatorDefault). They are
  # added per target via -Wl,-framework,... below (CMAKE_OSX_SYSTEM_FRAMEWORKS is
  # not honored by this CMake on this machine).
endif()
set(FDB_SSL_LIB    ${FDB_OPENSSL_LIB_DIR}/lib/libssl.a)
set(FDB_CRYPTO_LIB ${FDB_OPENSSL_LIB_DIR}/lib/libcrypto.a)
if(NOT EXISTS ${FDB_LIB_DIR}/libfdb_c.a)
  message(FATAL_ERROR
    "FoundationDB C client not built at ${FDB_LIB_DIR}/libfdb_c.a.\n"
    "Build the fork first:\n"
    "  cmake -S ${FDB_SRC_DIR} -B ${FDB_BUILD_DIR} -DCMAKE_BUILD_TYPE=Release "
    "-DFDB_BUILD_FDB_C_STATIC=ON -DFDB_USE_CSHARP_TOOLS=OFF "
    "-DBUILD_PYTHON_BINDING=OFF -DBUILD_JAVA_BINDING=OFF -DBUILD_GO_BINDING=OFF "
    "-DBUILD_RUBY_BINDING=OFF -DBUILD_DOCUMENTATION=OFF "
    "-DOpenSSL_DIR=/opt/homebrew/opt/openssl@3 -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/openssl@3\n"
    "  cmake --build ${FDB_BUILD_DIR} --target fdb_c")
endif()
# Authoritative static link chain — the fork's own CMake link order. libfdb_c.a is a thin
# C wrapper; fdbclient is its private impl plus that impl's dependencies. (The fork also
# linked -framework IOKit/CoreFoundation for its executables; the C client chain needs neither.)
set(FDB_STATIC_CHAIN
  ${FDB_LIB_DIR}/libfdb_c.a
  ${FDB_LIB_DIR}/libfdbclient.a
  ${FDB_LIB_DIR}/libfdbrpc.a
  ${FDB_LIB_DIR}/libflow.a
  ${FDB_LIB_DIR}/libstacktrace.a
  ${FDB_LIB_DIR}/libfmt.a
  ${FDB_LIB_DIR}/libcrc32.a
  ${FDB_SSL_LIB}
  ${FDB_CRYPTO_LIB}
  ${FDB_BOOST_LIB}/libboost_context.a
  ${FDB_BOOST_LIB}/libboost_filesystem.a
  ${FDB_BOOST_LIB}/libboost_iostreams.a
  ${FDB_BOOST_LIB}/libboost_system.a
  ${FDB_BOOST_LIB}/libboost_serialization.a
  ${FDB_LIB_DIR}/liblibb64.a
  ${FDB_LIB_DIR}/libmd5.a
  ${FDB_LIB_DIR}/libeio.a
  stdc++)

add_library(weft_fdb_vfs STATIC fdb_vfs.c)
# Position-independent, so the library can be linked into a shared object.
set_property(TARGET weft_fdb_vfs PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(weft_fdb_vfs PUBLIC
  ${FDB_INCLIB} ${FDB_GEN_INCLIB} ${SQLITE_INCLUDE_DIR})
target_compile_definitions(weft_fdb_vfs PUBLIC FDB_API_VERSION=730)
target_link_libraries(weft_fdb_vfs PUBLIC ${FDB_STATIC_CHAIN} ${SQLITE_LIBRARY} Threads::Threads)
if(APPLE)
  # IOKit/CoreFoundation are needed by the static FDB chain's platform code; propagate to
  # every consumer of weft_fdb_vfs so the frameworks land on each executable's link line.
  target_link_options(weft_fdb_vfs PUBLIC -Wl,-framework,IOKit -Wl,-framework,CoreFoundation)
endif()

# The VFS as a loadable extension for a host that brings its own SQLite (exqlite in the BEAM).
# fdb_vfs_ext.c compiles fdb_vfs.c against sqlite3ext.h, so the module links no SQLite.
add_library(weftfdb MODULE fdb_vfs_ext.c)
set_target_properties(weftfdb PROPERTIES PREFIX "" POSITION_INDEPENDENT_CODE ON)
if(APPLE)
  set_target_properties(weftfdb PROPERTIES SUFFIX ".dylib")
endif()
target_include_directories(weftfdb PRIVATE
  ${FDB_INCLIB} ${FDB_GEN_INCLIB} ${SQLITE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(weftfdb PRIVATE FDB_API_VERSION=730)
target_link_libraries(weftfdb PRIVATE ${FDB_STATIC_CHAIN} Threads::Threads)
if(APPLE)
  # IOKit/CoreFoundation needed by the static FDB chain's platform code.
  target_link_options(weftfdb PRIVATE -Wl,-framework,IOKit -Wl,-framework,CoreFoundation)
endif()

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
