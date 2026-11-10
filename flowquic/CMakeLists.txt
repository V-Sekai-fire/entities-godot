# Rung A of HTTP/3 on Flow: picoquic driven by Net2 over IUDPSocket. Off by default (option FLOWQUIC).
# TLS here is OpenSSL, which flow already links. RFD66606-2261 wants mbedTLS; that is a recorded deviation.
set(FLOWQUIC_PICOQUIC_DIR "" CACHE PATH "picoquic source tree; blank fetches FLOWQUIC_PICOQUIC_TAG")
set(FLOWQUIC_PICOQUIC_TAG "a6f85e8a73a6e8d9bc2143ebacb4f218ed4c0caf" CACHE STRING "picoquic commit to fetch")

set(picoquic_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PICO_SIM OFF CACHE BOOL "" FORCE)
set(BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(BUILD_HTTP ON CACHE BOOL "" FORCE) # h3zero and WebTransport (rung B)
set(BUILD_PQBENCH OFF CACHE BOOL "" FORCE)
set(BUILD_LOGREADER OFF CACHE BOOL "" FORCE)
set(PICOQUIC_FETCH_PTLS ON CACHE BOOL "" FORCE)
set(WITH_OPENSSL ON CACHE BOOL "" FORCE)

if(FLOWQUIC_PICOQUIC_DIR)
  add_subdirectory(${FLOWQUIC_PICOQUIC_DIR} picoquic EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(picoquic_src GIT_REPOSITORY https://github.com/private-octopus/picoquic.git GIT_TAG ${FLOWQUIC_PICOQUIC_TAG})
  FetchContent_GetProperties(picoquic_src)
  if(NOT picoquic_src_POPULATED)
    FetchContent_Populate(picoquic_src)
    add_subdirectory(${picoquic_src_SOURCE_DIR} ${picoquic_src_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

# picotls's own CMake has no Windows handling. Its sources include "wincompat.h" under _WINDOWS, and
# that header (with wintimeofday.c, the gettimeofday it maps to) lives in picotlsvs/picotls/.
if(WIN32 AND TARGET picotls-core)
  get_target_property(_ptls_dir picotls-core SOURCE_DIR)
  set(_ptls_win "${_ptls_dir}/picotlsvs/picotls")
  if(NOT EXISTS "${_ptls_win}/wincompat.h")
    message(FATAL_ERROR "picotls has no ${_ptls_win}/wincompat.h; the pinned picotls changed")
  endif()
  target_sources(picotls-core PRIVATE "${_ptls_win}/wintimeofday.c")
  # Force-included into every C file that sees picotls.h; see the header for why.
  target_compile_options(picotls-core PUBLIC
    "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:C>:/FI${CMAKE_CURRENT_SOURCE_DIR}/ptls_clangcl_compat.h>>")
  # The compat header must leave picotls's array check enforced, not stubbed out. Compile the real
  # macro with the same flags: an array must be accepted and a pointer refused. The refusals prove
  # nothing alone (a missing include also fails), so the array case has to pass first.
  include(CheckCSourceCompiles)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_FLAGS "/FI${CMAKE_CURRENT_SOURCE_DIR}/ptls_clangcl_compat.h")
  set(CMAKE_REQUIRED_DEFINITIONS -D_WINDOWS)
  set(CMAKE_REQUIRED_INCLUDES "${_ptls_dir}/include" "${_ptls_win}")
  check_c_source_compiles("#include <picotls.h>
int main(void) { unsigned char a[4]; return (int)PTLS_ELEMENTSOF(a); }" FLOWQUIC_PTLS_ARRAY_ACCEPTED)
  check_c_source_compiles("#include <picotls.h>
int main(void) { unsigned char *p = 0; return (int)PTLS_ELEMENTSOF(p); }" FLOWQUIC_PTLS_POINTER_ACCEPTED)
  check_c_source_compiles("#include <picotls.h>
int main(void) { return PTLS_BUILD_ASSERT_EXPR(0); }" FLOWQUIC_PTLS_FALSE_ASSERT_ACCEPTED)
  cmake_pop_check_state()
  if(NOT FLOWQUIC_PTLS_ARRAY_ACCEPTED)
    message(FATAL_ERROR "PTLS_ELEMENTSOF refuses an array under the compat header; the check cannot tell anything")
  endif()
  if(FLOWQUIC_PTLS_POINTER_ACCEPTED)
    message(FATAL_ERROR "PTLS_ELEMENTSOF accepts a pointer: picotls's array check is not enforced under clang-cl")
  endif()
  if(FLOWQUIC_PTLS_FALSE_ASSERT_ACCEPTED)
    message(FATAL_ERROR "PTLS_BUILD_ASSERT_EXPR(0) compiles: the build assertion is not enforced under clang-cl")
  endif()
  # cifra/bitops.h passes a uint32_t* to _BitScanReverse, which takes unsigned long*. They are both
  # 32 bits on Windows, and clang 22 makes the mismatch an error. Kept as a warning, for minicrypto only.
  if(TARGET picotls-minicrypto)
    target_compile_options(picotls-minicrypto PRIVATE
      "$<$<COMPILE_LANGUAGE:C>:-Wno-error=incompatible-pointer-types>")
  endif()
  foreach(_t picotls-core picotls-openssl picotls-minicrypto)
    if(TARGET ${_t})
      # BUILD_INTERFACE: the fetched source sits in the build tree, and picotls exports these targets,
      # so CMake refuses a bare build-tree path in their interface includes.
      target_include_directories(${_t} PUBLIC "$<BUILD_INTERFACE:${_ptls_win}>")
    endif()
  endforeach()
  # cifra's count_trailing_zeroes computed 31 - _BitScanReverse(x) on Windows, which finds the
  # HIGHEST set bit, so the result was a leading-zero count, not trailing. Measured (the probe
  # below) 7 of 7 planted answers wrong, e.g. count_trailing_zeroes(1) returned 31, not 0. Nothing
  # in this build calls it (picotls never compiles ocb.c), so this is a configure-time source
  # patch, not a behaviour change here. _BitScanForward gives the index of the LOWEST set bit,
  # which is exactly a trailing-zero count, so the fix is to call that and return its result
  # directly. Patched in the fetched tree, not upstream, so a tag bump that already fixed this
  # fails loudly (the REPLACE finding nothing) rather than double-patching or silently no-op'ing.
  set(_bitops "${_ptls_dir}/deps/cifra/src/bitops.h")
  if(NOT EXISTS "${_bitops}")
    message(FATAL_ERROR "picotls has no ${_bitops}; the pinned picotls changed")
  endif()
  file(READ "${_bitops}" _bitops_src)
  set(_ctz_broken [[  uint32_t r = 0;
  _BitScanReverse(&r, x);
  return (31 - r);]])
  set(_ctz_fixed [[  uint32_t r = 0;
  _BitScanForward(&r, x);
  return r;]])
  string(FIND "${_bitops_src}" "${_ctz_broken}" _ctz_at)
  if(_ctz_at EQUAL -1)
    message(FATAL_ERROR "cifra's _WINDOWS count_trailing_zeroes body did not match the expected text; the pinned picotls changed and this patch needs re-checking")
  endif()
  string(REPLACE "${_ctz_broken}" "${_ctz_fixed}" _bitops_src "${_bitops_src}")
  file(WRITE "${_bitops}" "${_bitops_src}")
endif()

# Probe for cifra's count_trailing_zeroes as bitops.h compiles it on Windows. Built on request
# (EXCLUDE_FROM_ALL); the portability workflow builds it by name and reports its numbers.
if(WIN32 AND DEFINED _ptls_dir)
  add_executable(ptls_cifra_ctz_probe EXCLUDE_FROM_ALL ptls_cifra_ctz_probe.c)
  target_include_directories(ptls_cifra_ctz_probe PRIVATE "${_ptls_dir}/deps/cifra/src")
  target_compile_definitions(ptls_cifra_ctz_probe PRIVATE _WINDOWS)
  target_compile_options(ptls_cifra_ctz_probe PRIVATE -Wno-error=incompatible-pointer-types)
endif()

add_flow_target(EXECUTABLE NAME flow_quic_echo SRCS QuicEcho.actor.cpp QuicDriver.cpp)
target_include_directories(flow_quic_echo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(flow_quic_echo PUBLIC flow picohttp-core picoquic-core)

# Unit tests of the run judge: no sockets, no connection, no Flow. Run directly by the portability
# workflow; there is no ctest here because the root build sets BUILD_TESTING=OFF.
add_executable(flowquic_unit_tests QuicUnitTests.cpp QuicDriver.cpp)
target_include_directories(flowquic_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(flowquic_unit_tests PRIVATE picohttp-core picoquic-core)
if(WIN32)
  # picotls's random source is BCryptGenRandom; flow_quic_echo gets bcrypt through flow.
  target_link_libraries(flowquic_unit_tests PRIVATE ws2_32 bcrypt)
endif()
