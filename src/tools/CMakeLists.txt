add_executable (sandbox sandbox.cpp)

target_link_libraries (sandbox PRIVATE
    utility
    io
    arg
    ui
)

target_include_directories(sandbox PRIVATE
    ${CMAKE_HOME_DIRECTORY}/include
    ../ncx
    ../ui
)

add_executable (bench bench.cpp)
target_link_libraries(bench benchmark::benchmark)

add_executable (worklog worklog.cpp)

target_link_libraries (worklog PRIVATE
    utility
    arg
    date
)

target_include_directories(worklog PRIVATE
    ${CMAKE_HOME_DIRECTORY}/include
    ../ncx
)

if (UNIX)
    target_link_libraries(sandbox PRIVATE pthread)
    target_link_libraries(worklog PRIVATE pthread)
endif()
