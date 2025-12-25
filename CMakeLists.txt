cmake_minimum_required(VERSION 3.16)
project(qwiet C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform selection (default to linux)
set(PLATFORM "linux" CACHE STRING "Target platform (linux, pinenote)")
set_property(CACHE PLATFORM PROPERTY STRINGS linux pinenote)

if(NOT PLATFORM MATCHES "^(linux|pinenote)$")
    message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM}. Must be 'linux' or 'pinenote'")
endif()

message(STATUS "Building for platform: ${PLATFORM}")

# Platform-specific subdirectory
add_subdirectory(platform/${PLATFORM})

# Test support library (cmake -DBUILD_TESTING=ON)
option(BUILD_TESTING "Build test support" OFF)
if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(platform/testing/diode)
    add_subdirectory(tests/diode)
    add_subdirectory(tests/list)
    add_subdirectory(tests/macros)
    add_subdirectory(tests/sem)
    add_subdirectory(tests/timer)
endif()
