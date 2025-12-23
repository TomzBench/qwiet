# libevdev - input device event handling library
# https://gitlab.freedesktop.org/libevdev/libevdev
#
# Usage:
#   find_package(Libevdev REQUIRED)
#   target_link_libraries(my_target PRIVATE libevdev::libevdev)
#
# Requires: meson, ninja

include(ExternalProject)

# Find meson - prefer project venv
find_program(MESON_EXECUTABLE meson HINTS ${CMAKE_SOURCE_DIR}/.venv/bin REQUIRED)

set(LIBEVDEV_VERSION "1.13.6")
set(LIBEVDEV_PREFIX "${CMAKE_BINARY_DIR}/external/libevdev")
set(LIBEVDEV_INSTALL_DIR "${LIBEVDEV_PREFIX}/install")
set(LIBEVDEV_INCLUDE_DIR "${LIBEVDEV_INSTALL_DIR}/include/libevdev-1.0")
set(LIBEVDEV_LIBRARY "${LIBEVDEV_INSTALL_DIR}/lib/libevdev.a")
set(LIBEVDEV_SOURCE_DIR "${LIBEVDEV_PREFIX}/src/libevdev_external")
set(LIBEVDEV_BUILD_DIR "${LIBEVDEV_SOURCE_DIR}-build")

set(LIBEVDEV_MESON_ARGS
    --prefix=${LIBEVDEV_INSTALL_DIR}
    --libdir=lib
    --default-library=static
    --buildtype=release
    -Dtests=disabled
    -Ddocumentation=disabled
    -Dcoverity=false
)

ExternalProject_Add(
    libevdev_external
    GIT_REPOSITORY https://gitlab.freedesktop.org/libevdev/libevdev.git
    GIT_TAG libevdev-${LIBEVDEV_VERSION}
    GIT_SHALLOW TRUE
    PREFIX ${LIBEVDEV_PREFIX}
    CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup ${LIBEVDEV_MESON_ARGS} ${LIBEVDEV_SOURCE_DIR} ${LIBEVDEV_BUILD_DIR}
    BUILD_COMMAND ninja -C ${LIBEVDEV_BUILD_DIR}
    INSTALL_COMMAND ninja -C ${LIBEVDEV_BUILD_DIR} install
    BUILD_BYPRODUCTS ${LIBEVDEV_LIBRARY}
)

# Create include directory at configure time (populated at build time)
file(MAKE_DIRECTORY ${LIBEVDEV_INCLUDE_DIR})

# Create imported target
add_library(libevdev::libevdev STATIC IMPORTED GLOBAL)
set_target_properties(
    libevdev::libevdev PROPERTIES IMPORTED_LOCATION ${LIBEVDEV_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES
                                                                        ${LIBEVDEV_INCLUDE_DIR}
)

# Ensure ExternalProject builds before anything uses the target
add_dependencies(libevdev::libevdev libevdev_external)
