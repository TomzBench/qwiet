# Unity/CMock Test Helpers
#
# Provides functions for Unity test runner generation and CMock mocking.
# Automatically included by find_package(CMock).
#
# Functions:
#   test_runner_generate(target test_file) - Create test executable with runner
#   cmock_handle(target header)            - Generate mock and add --wrap linker options
#   cmock_generate(target header)          - Generate mock for header
#   cmock_linker_wrap(target header)       - Add --wrap linker options

include_guard(GLOBAL)

# Find required tools
find_program(RUBY_EXECUTABLE ruby REQUIRED)
find_program(PYTHON_EXECUTABLE python3 REQUIRED)

# Python tools (GLOBAL so functions can access)
set_property(GLOBAL PROPERTY CMOCK_HEADER_PREPARE_PY "${CMAKE_SOURCE_DIR}/tools/unity/header_prepare.py")
set_property(GLOBAL PROPERTY CMOCK_FUNC_NAME_LIST_PY "${CMAKE_SOURCE_DIR}/tools/unity/func_name_list.py")

# Config files (GLOBAL so functions can access)
set_property(GLOBAL PROPERTY CMOCK_CFG_FILE "${CMAKE_SOURCE_DIR}/cmake/modules/unity_cfg.yaml")
set_property(GLOBAL PROPERTY CMOCK_CONFIG_H "${CMAKE_SOURCE_DIR}/cmake/modules/unity_config.h")
set_property(GLOBAL PROPERTY CMOCK_GENERIC_TEARDOWN_C "${CMAKE_SOURCE_DIR}/platform/testing/${PLATFORM}/generic_teardown.c")

# Mock output directory (GLOBAL so functions can access)
set(CMOCK_PRODUCTS_DIR "${CMAKE_BINARY_DIR}/cmock_products")
set_property(GLOBAL PROPERTY CMOCK_PRODUCTS_DIR "${CMOCK_PRODUCTS_DIR}")
file(MAKE_DIRECTORY ${CMOCK_PRODUCTS_DIR})
file(MAKE_DIRECTORY ${CMOCK_PRODUCTS_DIR}/internal)

# Create Unity test executable with generated runner
#
# Usage:
#   test_runner_generate(my_test src/test.c)
#   target_link_libraries(my_test PRIVATE my_lib)
#
# Creates executable target and registers it with CTest.
function(test_runner_generate TARGET TEST_FILE)
    get_property(CMOCK_GENERATE_TEST_RUNNER GLOBAL PROPERTY CMOCK_GENERATE_TEST_RUNNER)
    get_property(CMOCK_CFG_FILE GLOBAL PROPERTY CMOCK_CFG_FILE)
    get_property(CMOCK_GENERIC_TEARDOWN_C GLOBAL PROPERTY CMOCK_GENERIC_TEARDOWN_C)

    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    set(RUNNER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_runner.c")

    get_filename_component(TEST_FILE_ABS ${TEST_FILE} ABSOLUTE)

    add_custom_command(
        OUTPUT ${RUNNER_FILE}
        COMMAND ${RUBY_EXECUTABLE} ${CMOCK_GENERATE_TEST_RUNNER} ${CMOCK_CFG_FILE} ${TEST_FILE_ABS} ${RUNNER_FILE}
        DEPENDS ${TEST_FILE_ABS} ${CMOCK_CFG_FILE}
        COMMENT "Generating test runner for ${TEST_NAME}"
        VERBATIM
    )

    add_executable(${TARGET} ${CMOCK_GENERIC_TEARDOWN_C} ${RUNNER_FILE} ${TEST_FILE})
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endfunction()

# Extract function names from header and add --wrap linker options
#
# Usage:
#   cmock_linker_wrap(my_target src/stubs.h)
#
# Adds -Wl,--wrap=func1,--wrap=func2,... to target's link options.
# Uses PUBLIC visibility so wrap flags propagate when library is linked.
function(cmock_linker_wrap TARGET HEADER)
    get_property(CMOCK_FUNC_NAME_LIST_PY GLOBAL PROPERTY CMOCK_FUNC_NAME_LIST_PY)
    get_property(CMOCK_PRODUCTS_DIR GLOBAL PROPERTY CMOCK_PRODUCTS_DIR)

    get_filename_component(HEADER_NAME ${HEADER} NAME_WE)
    get_filename_component(HEADER_ABS ${HEADER} ABSOLUTE)

    set(FUNC_LIST_FILE "${CMOCK_PRODUCTS_DIR}/${HEADER_NAME}.flist")

    # Generate function list at configure time
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${CMOCK_FUNC_NAME_LIST_PY} -i ${HEADER_ABS} -o ${FUNC_LIST_FILE}
        RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract function names from ${HEADER}")
    endif()

    # Read function names and build linker wrap string
    file(STRINGS ${FUNC_LIST_FILE} FUNC_NAMES)
    set(WRAP_FLAGS "")
    foreach(FUNC ${FUNC_NAMES})
        string(STRIP "${FUNC}" FUNC)
        if(NOT "${FUNC}" STREQUAL "")
            list(APPEND WRAP_FLAGS "-Wl,--wrap=${FUNC}")
        endif()
    endforeach()

    if(WRAP_FLAGS)
        target_link_options(${TARGET} PUBLIC ${WRAP_FLAGS})
    endif()
endfunction()

# Prepare header for CMock mock generation
#
# Usage:
#   cmock_headers_prepare(src/stubs.h)
#
# Strips comments, static inlines, and creates wrapped version with __wrap_ prefix.
# Sets CMOCK_HEADER_STRIPPED and CMOCK_HEADER_WRAPPED in parent scope.
function(cmock_headers_prepare HEADER)
    get_property(CMOCK_HEADER_PREPARE_PY GLOBAL PROPERTY CMOCK_HEADER_PREPARE_PY)
    get_property(CMOCK_PRODUCTS_DIR GLOBAL PROPERTY CMOCK_PRODUCTS_DIR)

    get_filename_component(HEADER_NAME ${HEADER} NAME)
    get_filename_component(HEADER_ABS ${HEADER} ABSOLUTE)

    set(STRIPPED_HEADER "${CMOCK_PRODUCTS_DIR}/internal/${HEADER_NAME}")
    set(WRAPPED_HEADER "${CMOCK_PRODUCTS_DIR}/internal/__wrap_${HEADER_NAME}")

    # Generate prepared headers at configure time
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${CMOCK_HEADER_PREPARE_PY} -i ${HEADER_ABS} -o ${STRIPPED_HEADER} -w
                ${WRAPPED_HEADER} RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to prepare header ${HEADER}")
    endif()

    set(CMOCK_HEADER_STRIPPED ${STRIPPED_HEADER} PARENT_SCOPE)
    set(CMOCK_HEADER_WRAPPED ${WRAPPED_HEADER} PARENT_SCOPE)
endfunction()

# Generate CMock mock from header and add to target
#
# Usage:
#   cmock_generate(my_target src/stubs.h)
#
# Generates unity_mock_*.c from the header and adds it to target sources.
# Also adds include directories for mock headers.
function(cmock_generate TARGET HEADER)
    get_property(CMOCK_RB GLOBAL PROPERTY CMOCK_RB)
    get_property(CMOCK_CFG_FILE GLOBAL PROPERTY CMOCK_CFG_FILE)
    get_property(CMOCK_PRODUCTS_DIR GLOBAL PROPERTY CMOCK_PRODUCTS_DIR)

    # Prepare the header (strips comments, creates __wrap_ version)
    cmock_headers_prepare(${HEADER})

    get_filename_component(HEADER_NAME ${HEADER} NAME_WE)

    # CMock outputs unity_mock_<name>.c and unity_mock_<name>.h
    set(MOCK_SOURCE "${CMOCK_PRODUCTS_DIR}/unity_mock___wrap_${HEADER_NAME}.c")
    set(MOCK_HEADER "${CMOCK_PRODUCTS_DIR}/unity_mock___wrap_${HEADER_NAME}.h")

    # Generate mock at configure time using CMock's lib/cmock.rb directly
    # NOTE: mock_prefix must match unity_cfg.yaml
    set(MOCK_PREFIX "unity_mock_")
    execute_process(
        COMMAND ${RUBY_EXECUTABLE} ${CMOCK_RB} --mock_prefix=${MOCK_PREFIX} --mock_path=${CMOCK_PRODUCTS_DIR}
                -o${CMOCK_CFG_FILE} ${CMOCK_HEADER_WRAPPED} RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate mock for ${HEADER}")
    endif()

    # Add mock source to target
    target_sources(${TARGET} PRIVATE ${MOCK_SOURCE})

    # Add include directories for mock headers (PUBLIC so tests can include)
    # Include both the main products dir and internal dir for generated headers
    target_include_directories(${TARGET} PUBLIC ${CMOCK_PRODUCTS_DIR} ${CMOCK_PRODUCTS_DIR}/internal)
endfunction()

# Generate mock and add --wrap linker options (convenience wrapper)
#
# Usage:
#   cmock_handle(my_target src/stubs.h)
#
# Combines cmock_generate() and cmock_linker_wrap() for full mock setup.
# Works for both library and executable targets.
function(cmock_handle TARGET HEADER)
    cmock_generate(${TARGET} ${HEADER})
    cmock_linker_wrap(${TARGET} ${HEADER})
endfunction()
