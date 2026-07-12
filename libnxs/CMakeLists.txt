file(GLOB_RECURSE NXS_HEADER_FILES "*.hpp")

add_library(nxs
    tty.cpp
    rlog.cpp
)

target_sources(nxs PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${NEXUS_PROJECT_ROOT}"
    FILES "${NXS_HEADER_FILES}"
)

target_include_directories(nxs PUBLIC
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nxs PUBLIC
    nova::nova
    fmt::fmt-header-only
    spdlog::spdlog_header_only
)

include(test.cmake)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_test_target(nxs)
endif()

add_executable(example-rlog rlog.ex.cpp)
target_link_libraries(example-rlog PRIVATE nxs)

install(TARGETS nxs
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
