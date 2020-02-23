add_executable(test_datetime test_datetime.cpp)

target_include_directories(test_datetime PRIVATE ../glib)

target_link_libraries(test_datetime PRIVATE date)

#add_test(test_datetime test_datetime)

#target_include_directories(test_datetime PRIVATE ../../Catch2/include)
#find_package(Catch2 CONFIG REQUIRED)
