find_package(CMock REQUIRED)

test_runner_generate(test_diode src/test.c)

target_include_directories(test_diode PRIVATE src)
target_link_libraries(test_diode PRIVATE curriculo_diode)

cmock_handle(test_diode src/stubs.h)
