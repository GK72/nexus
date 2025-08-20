add_executable(baldr
    main.cpp
)

target_link_libraries(baldr PRIVATE nova::nova boost::boost)
