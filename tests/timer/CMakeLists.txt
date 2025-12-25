find_package(CMock REQUIRED)

test_runner_generate(test_timer src/test.c)

target_include_directories(test_timer PRIVATE src)
target_include_directories(test_timer PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_timer PRIVATE qwiet_pal unity)
