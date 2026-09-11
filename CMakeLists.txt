# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)
project(witness-cpp VERSION 0.1.0 LANGUAGES CXX)

# Header-only interface library. Downstream:
#   add_subdirectory(witness-cpp)
#   target_link_libraries(mytests PRIVATE witness::witness)
add_library(witness_lib INTERFACE)
add_library(witness::witness ALIAS witness_lib)

target_include_directories(witness_lib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_compile_features(witness_lib INTERFACE cxx_std_17)

option(WITNESS_BUILD_TESTS "Build witness-cpp tests" ${PROJECT_IS_TOP_LEVEL})
option(WITNESS_NO_EXCEPTIONS "Build tests with -fno-exceptions" ON)

if(WITNESS_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# Install rules — pkg users can `find_package(witness-cpp)`.
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS witness_lib EXPORT witness-cpp-targets)
install(EXPORT witness-cpp-targets
	FILE witness-cpp-targets.cmake
	NAMESPACE witness::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/witness-cpp
)
