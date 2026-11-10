add_library(heimdall-core
    config.cpp
    dashboard.cpp
    git.cpp
    heimdall.cpp
    process.cpp
    report.cpp
    style.cpp
)

target_include_directories(heimdall-core PUBLIC
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
)

target_link_libraries(heimdall-core PUBLIC
    nxs
    nova::nova
    fmt::fmt-header-only
)

add_test_target(heimdall-core)

add_executable(heimdall main.cpp)
target_link_libraries(heimdall PRIVATE heimdall-core)

install(TARGETS heimdall RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
