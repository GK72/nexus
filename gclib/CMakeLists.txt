add_executable(gclib_main main.c)

add_library(filesystem fs.c)

target_link_libraries(gclib_main PRIVATE filesystem)