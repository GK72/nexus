file(GLOB_RECURSE LEXY_HEADER_FILES "*.hpp")

add_library(lexy
    descriptor.cpp
    lxy_parser.cpp
    decoder.cpp
    generator.cpp
)

target_sources(lexy PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${NEXUS_PROJECT_ROOT}"
    FILES "${LEXY_HEADER_FILES}"
)

target_include_directories(lexy PUBLIC
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(lexy PUBLIC
    btx
    nova::nova
    yaml-cpp
)

add_test_target(lexy)

install(TARGETS lexy
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
