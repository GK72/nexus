add_executable (main Console.cpp)

target_link_libraries (main PRIVATE utility)
target_link_libraries (main PRIVATE io)
target_link_libraries (main PRIVATE arg)
target_link_libraries (main PRIVATE data)
target_include_directories(main PRIVATE ../glib)

add_executable (worklog worklog.cpp)

target_link_libraries (worklog PRIVATE utility)
target_link_libraries (worklog PRIVATE arg)
target_link_libraries (worklog PRIVATE date)
target_include_directories(worklog PRIVATE ${CMAKE_HOME_DIRECTORY}/include)
target_include_directories(worklog PRIVATE ../glib)

if (UNIX)
    target_link_libraries(main PRIVATE pthread)
    target_link_libraries(worklog PRIVATE pthread)
endif()
