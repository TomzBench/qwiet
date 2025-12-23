find_package(Unity REQUIRED)
find_package(CMock REQUIRED)

add_library(curriculo_diode src/poll.c)

target_include_directories(curriculo_diode PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(curriculo_diode PUBLIC unity cmock)
