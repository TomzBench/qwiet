# CMock - Mock Generation for Unity Tests
# https://github.com/ThrowTheSwitch/CMock
#
# Usage:
#   find_package(CMock REQUIRED)
#   test_runner_generate(my_test src/test.c)
#   target_link_libraries(my_test PRIVATE my_lib)
#   cmock_handle(my_test src/stubs.h)
#
# Creates target: cmock
# Includes: unity.cmake (helper functions)

# Locate CMock directory (sibling to this project)
set(CMOCK_DIR "${CMAKE_SOURCE_DIR}/../cmock" CACHE PATH "Path to CMock")

if(NOT EXISTS "${CMOCK_DIR}")
    message(FATAL_ERROR "CMock not found at ${CMOCK_DIR}")
endif()

set(CMOCK_UNITY_DIR "${CMOCK_DIR}/vendor/unity")
set(CMOCK_UNITY_AUTO_DIR "${CMOCK_UNITY_DIR}/auto")
set(CMOCK_LIB_DIR "${CMOCK_DIR}/lib")
set(CMOCK_SRC_DIR "${CMOCK_DIR}/src")

# Ruby scripts (GLOBAL so unity.cmake functions can access them)
set(CMOCK_GENERATE_TEST_RUNNER "${CMOCK_UNITY_AUTO_DIR}/generate_test_runner.rb")
set_property(GLOBAL PROPERTY CMOCK_GENERATE_TEST_RUNNER "${CMOCK_GENERATE_TEST_RUNNER}")
set_property(GLOBAL PROPERTY CMOCK_RB "${CMOCK_LIB_DIR}/cmock.rb")

if(NOT EXISTS "${CMOCK_GENERATE_TEST_RUNNER}")
    message(FATAL_ERROR "generate_test_runner.rb not found at ${CMOCK_GENERATE_TEST_RUNNER}")
endif()

# Create CMock library target
if(NOT TARGET cmock)
    find_package(Unity REQUIRED)
    add_library(cmock STATIC ${CMOCK_SRC_DIR}/cmock.c)
    target_include_directories(cmock PUBLIC ${CMOCK_SRC_DIR})
    target_link_libraries(cmock PUBLIC unity)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(cmock PRIVATE -w)
    endif()
endif()

# Include helper functions (test_runner_generate, cmock_handle, etc.)
include(unity)
