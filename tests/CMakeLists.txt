add_executable(test_nexus
    test.cpp
    test_allocator.cpp
    test_datetime.cpp
    test_detector.cpp
    test_io_tokenizer.cpp
    test_units.cpp
    test_utility.cpp
    test_linkedList.cpp
)

target_include_directories(test_nexus PRIVATE ${CMAKE_HOME_DIRECTORY}/include)
target_include_directories(test_nexus PRIVATE ../src/ncx)

target_link_libraries(test_nexus date)
target_link_libraries(test_nexus io)
target_link_libraries(test_nexus utility)

target_compile_definitions(test_nexus PRIVATE __PATH_ROOT_REPO="${CMAKE_HOME_DIRECTORY}")

include(CTest)
include(Catch)
catch_discover_tests(test_nexus)
