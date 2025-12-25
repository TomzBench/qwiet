find_package(Libevdev REQUIRED)

add_library(qwiet_pal src/event.c src/input_evdev.c src/net.c src/sem.c src/time.c src/timer.c)
target_include_directories(qwiet_pal PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(qwiet_pal PUBLIC PLATFORM_LINUX)
target_link_libraries(qwiet_pal PUBLIC libevdev::libevdev)
