find_package(Libevdev REQUIRED)

add_library(curriculo_pal src/event.c src/net.c)

target_include_directories(curriculo_pal PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(curriculo_pal PRIVATE PLATFORM_LINUX)

target_link_libraries(curriculo_pal PRIVATE libevdev::libevdev)
