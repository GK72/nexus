file(GLOB_RECURSE BTX_HEADER_FILES "*.hpp" "*.h")

add_library(btx
    btx.cpp
)

add_library(btx_c SHARED
    btx_cbindings.cpp
)

target_link_libraries(btx_c PRIVATE btx)

target_sources(btx PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${NEXUS_PROJECT_ROOT}"
    FILES "${BTX_HEADER_FILES}"
)

target_include_directories(btx PUBLIC
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(btx PUBLIC
    nova::nova
)

include(test.cmake)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_test_target(btx)
endif()

install(TARGETS btx btx_c
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
