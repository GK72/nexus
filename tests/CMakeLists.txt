add_executable(test_glib
    test_io.cpp
)

add_test(test_glib test_glib)

target_include_directories(test_glib PRIVATE ../include)
target_include_directories(test_glib PRIVATE ../src/glib)

target_link_libraries(test_glib io)


find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(test_glib Catch2::Catch2)
