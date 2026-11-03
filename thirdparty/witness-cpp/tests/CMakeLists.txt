# SPDX-License-Identifier: MIT
include(FetchContent)

# doctest v2.4.11's top-level CMakeLists requires cmake < 3.5, which
# recent CMake removed. Passing -DCMAKE_POLICY_VERSION_MINIMUM=3.5 into
# the FetchContent build lets it configure cleanly on CMake 4.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
	doctest
	GIT_REPOSITORY https://github.com/doctest/doctest.git
	GIT_TAG        v2.4.11
	GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

add_executable(witness_lib_tests
	main.cpp
	test_witness.cpp
	test_generators.cpp
	test_shrinkers.cpp
	test_reproducer.cpp
	test_assume_classify.cpp
	test_combinators.cpp
)

target_link_libraries(witness_lib_tests PRIVATE
	witness::witness
	doctest::doctest
)

# Prove exception-free: build the whole test binary with -fno-exceptions
# (matching Godot's default). If any dependency of witness/ladder.h
# needed throw/try/catch, this build would fail. This is the guarantee
# that lets downstream projects embed the header inside a build with
# exceptions disabled.
if(WITNESS_NO_EXCEPTIONS)
	if(MSVC)
		# MSVC's STL throws even with /EHs- so this doesn't apply on MSVC;
		# leave the default flags in place there.
	else()
		target_compile_options(witness_lib_tests PRIVATE
			-fno-exceptions
			-fno-rtti
		)
		# doctest defines DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
		# so its own asserts don't rely on throw.
		target_compile_definitions(witness_lib_tests PRIVATE
			DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
		)
	endif()
endif()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(witness_lib_tests)
