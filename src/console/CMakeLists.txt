add_executable (main Console.cpp)

target_link_libraries (main PRIVATE utility)
target_link_libraries (main PRIVATE io)
target_link_libraries (main PRIVATE arg)
target_link_libraries (main PRIVATE data)
target_include_directories(main PRIVATE ../glib)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(main PRIVATE pthread)
endif()
