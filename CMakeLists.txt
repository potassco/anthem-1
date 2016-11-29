cmake_minimum_required(VERSION 2.6)
project(anthem CXX)

option(ANTHEM_BUILD_TESTS "Build unit tests" OFF)
option(ANTHEM_BUILD_STATIC "Build static binaries" OFF)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(ANTHEM_BUILD_STATIC)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	# Remove -Wl,-Bdynamic
	set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)
	set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
endif()

set(CLASP_BUILD_APP OFF CACHE BOOL "whether or not to build the clasp application")
set(CLASP_BUILD_TEST OFF CACHE BOOL "whether or not to build clasp unit tests (requires CppUnit)")

if(ANTHEM_BUILD_STATIC)
	set(CLINGO_BUILD_SHARED OFF CACHE BOOL "build clingo library shared")
else()
	set(CLINGO_BUILD_SHARED ON CACHE BOOL "build clingo library shared")
endif()

set(CLINGO_BUILD_WITH_LUA OFF CACHE BOOL "enable lua support")
set(CLINGO_BUILD_WITH_PYTHON OFF CACHE BOOL "enable python support")
set(LIB_POTASSCO_BUILD_APP OFF CACHE BOOL "whether or not to build lpconvert tool")
set(LIB_POTASSCO_BUILD_TESTS OFF CACHE BOOL "whether or not to build tests")

add_subdirectory(lib/clingo)
add_subdirectory(src)
add_subdirectory(app)

if(ANTHEM_BUILD_TESTS)
	add_subdirectory(tests)
endif()
