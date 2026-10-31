# The property gate for `fdb_keys.h`, over `witness-cpp` and doctest.
#
# This directory replaces the previous `fuzztest/` peer. Where fuzztest pulled
# Clang, google/fuzztest, google/googletest and a network FetchContent, this
# pulls only header-only witness-cpp (which lives at `../../../5-repository/
# witness-cpp` in the workspace, with a FetchContent fallback for a standalone
# checkout) and doctest. It builds with `-fno-exceptions -fno-rtti` because the
# tree it belongs to does, and every property here is paired with a planted
# rule-2 control that asserts the wrong shape fails.
#
#   cmake -S keys-witness -B build-keys-witness -DCMAKE_BUILD_TYPE=RelWithDebInfo
#   cmake --build build-keys-witness -j
#   ctest --test-dir build-keys-witness --output-on-failure
#
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.19)
project(weft_store_keys_witness CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# witness-cpp lives at 5-repository/witness-cpp in the workspace. On a
# standalone checkout that path is absent, so fall back to the upstream mirror.
set(WEFT_WITNESS_CPP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../5-repository/witness-cpp")
if(IS_DIRECTORY "${WEFT_WITNESS_CPP_SRC}")
	add_subdirectory("${WEFT_WITNESS_CPP_SRC}" witness-cpp-build EXCLUDE_FROM_ALL)
else()
	FetchContent_Declare(
		witness_cpp
		GIT_REPOSITORY https://github.com/V-Sekai-fire/repository-witness-cpp.git
		GIT_TAG        main
	)
	FetchContent_MakeAvailable(witness_cpp)
endif()

# doctest v2.4.11 pins CMake < 3.5 at its top level; the same escape hatch
# `witness-cpp/tests/CMakeLists.txt` uses works here.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
	doctest
	GIT_REPOSITORY https://github.com/doctest/doctest.git
	GIT_TAG        v2.4.11
	GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

enable_testing()

add_executable(keys_test main.cpp keys_test.cc)
target_include_directories(keys_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(keys_test PRIVATE witness::witness doctest::doctest)
target_compile_options(keys_test PRIVATE -Wall -Wextra)

# Exception-free build, to prove the header is embeddable in a Godot / LLVM
# style tree that ships without exceptions. Same shape as
# witness-cpp/tests/CMakeLists.txt's WITNESS_NO_EXCEPTIONS block.
if(NOT MSVC)
	target_compile_options(keys_test PRIVATE -fno-exceptions -fno-rtti)
	target_compile_definitions(keys_test PRIVATE
		DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
endif()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(keys_test)
