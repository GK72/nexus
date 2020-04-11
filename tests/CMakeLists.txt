add_executable(test_glib
    test.cpp
    test_io_tokenizer.cpp
    test_io_json.cpp
)

target_include_directories(test_glib PRIVATE ${CMAKE_HOME_DIRECTORY}/include)
target_include_directories(test_glib PRIVATE ../src/glib)

target_link_libraries(test_glib io)


find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(test_glib Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(test_glib)
