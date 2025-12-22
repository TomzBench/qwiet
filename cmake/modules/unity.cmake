# Unity Test Framework
# https://github.com/ThrowTheSwitch/Unity
#
# Usage:
#   include(cmake/modules/unity.cmake)
#   target_link_libraries(my_test_target PRIVATE unity)

include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.6.0
    GIT_SHALLOW TRUE
)

# Unity doesn't have a CMakeLists.txt at root, so we build it ourselves
FetchContent_GetProperties(unity)
if(NOT unity_POPULATED)
    FetchContent_Populate(unity)

    # Create a library target for Unity
    add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)

    target_include_directories(unity PUBLIC ${unity_SOURCE_DIR}/src)

    # Optional: Unity configuration
    target_compile_definitions(
        unity
        PUBLIC
            # Uncomment/modify as needed:
            # UNITY_INCLUDE_DOUBLE          # Enable double support
            # UNITY_INCLUDE_FLOAT           # Enable float support (enabled by default)
            # UNITY_SUPPORT_64              # Enable 64-bit integer support
            # UNITY_OUTPUT_COLOR            # Colored output
            # UNITY_EXCLUDE_STDINT_H        # Don't include stdint.h
            # UNITY_EXCLUDE_LIMITS_H        # Don't include limits.h
            # UNITY_EXCLUDE_SETJMP_H        # Don't include setjmp.h (disables TEST_PROTECT)
    )

    # Suppress warnings in Unity source
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(unity PRIVATE -w)
    endif()
endif()

# Convenience function to create a Unity test executable
function(add_unity_test TEST_NAME)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES LIBRARIES)
    cmake_parse_arguments(
        ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    add_executable(${TEST_NAME} ${ARG_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE unity ${ARG_LIBRARIES})

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()
