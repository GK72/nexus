file(GLOB_RECURSE BTX_HEADER_FILES "*.hpp")

add_library(btx
    btx.cpp
    descriptor.cpp
    decoder.cpp
    generator.cpp
)

target_sources(btx PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${BTX_PROJECT_ROOT}"
    FILES "${BTX_HEADER_FILES}"
)

target_include_directories(btx PUBLIC
    $<BUILD_INTERFACE:${BTX_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(btx PUBLIC
    nova::nova
    yaml-cpp
)

include(test.cmake)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_test_target(btx)
endif()
