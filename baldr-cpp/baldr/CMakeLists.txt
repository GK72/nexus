add_executable(baldr
    main.cpp
    # docker.cpp
    progress.cpp
)

target_compile_definitions(baldr PRIVATE BALDR_TUI_DEBUG=1)
target_link_libraries(baldr PRIVATE nova::nova boost::boost)
target_include_directories(baldr PRIVATE "${BALDR_PROJECT_ROOT}")
