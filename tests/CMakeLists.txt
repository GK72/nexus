add_executable(test_datetime test_datetime.cpp)

target_include_directories(test_datetime PRIVATE ../glib)

target_link_libraries(test_datetime PRIVATE date)


add_executable(test_catch test.cpp)
add_test(test_catch test_catch)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(test_catch Catch2::Catch2)
