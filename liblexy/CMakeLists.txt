file(GLOB_RECURSE LEXY_HEADER_FILES "*.hpp")

add_library(lexy
    descriptor.cpp
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

include(test.cmake)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_test_target(lexy)
endif()

install(TARGETS lexy
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
