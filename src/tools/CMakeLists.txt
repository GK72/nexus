add_executable (sandbox sandbox.cpp)

target_link_libraries (sandbox PRIVATE utility)
target_link_libraries (sandbox PRIVATE io)
target_link_libraries (sandbox PRIVATE arg)
target_include_directories(sandbox PRIVATE ${CMAKE_HOME_DIRECTORY}/include)
target_include_directories(sandbox PRIVATE ../ncx)

add_executable (worklog worklog.cpp)

target_link_libraries (worklog PRIVATE utility)
target_link_libraries (worklog PRIVATE arg)
target_link_libraries (worklog PRIVATE date)
target_include_directories(worklog PRIVATE ${CMAKE_HOME_DIRECTORY}/include)
target_include_directories(worklog PRIVATE ../ncx)

if (UNIX)
    target_link_libraries(sandbox PRIVATE pthread)
    target_link_libraries(worklog PRIVATE pthread)
endif()
