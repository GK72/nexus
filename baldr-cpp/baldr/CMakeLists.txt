add_executable(baldr
    main.cpp
    builder.cpp
    config.cpp
    utils.cpp
    progress.cpp
    tty.cpp
    log.cpp
)

target_compile_definitions(baldr PRIVATE BALDR_TUI_DEBUG=1)
target_link_libraries(baldr PRIVATE nova::nova boost::boost yaml-cpp::yaml-cpp)
target_include_directories(baldr PRIVATE "${BALDR_PROJECT_ROOT}")

include(test.cmake)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    add_test_target(baldr)
endif()
