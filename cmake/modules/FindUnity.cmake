# Unity Test Framework
# https://github.com/ThrowTheSwitch/Unity
#
# Usage:
#   find_package(Unity REQUIRED)
#   target_link_libraries(my_test_target PRIVATE unity)
#
# Creates target: unity

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

    add_library(unity STATIC ${unity_SOURCE_DIR}/src/unity.c)

    target_include_directories(unity PUBLIC ${unity_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/cmake/modules)

    # Unity configuration - use unity_config.h for output settings
    target_compile_definitions(unity PUBLIC UNITY_INCLUDE_CONFIG_H)

    # Suppress warnings in Unity source
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(unity PRIVATE -w)
    endif()
endif()
