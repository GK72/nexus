file(GLOB_RECURSE DGM_HEADER_FILES "*.hpp")

add_library(dgm
    parser.cpp
    layout.cpp
    svg.cpp
)

target_sources(dgm PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${NEXUS_PROJECT_ROOT}"
    FILES "${DGM_HEADER_FILES}"
)

target_include_directories(dgm PUBLIC
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(dgm PUBLIC
    nova::nova
    fmt::fmt-header-only
)

add_test_target(dgm)

add_executable(dgm-tool main.cpp)
target_link_libraries(dgm-tool PRIVATE dgm Boost::program_options)

install(TARGETS dgm-tool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
