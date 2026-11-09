file(GLOB @NAME@_HEADER_FILES CONFIGURE_DEPENDS "*.hpp")

add_library(@name@-core
    greeting.cpp
)

target_sources(@name@-core PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${@NAME@_ROOT}/lib@name@"
    FILES ${@NAME@_HEADER_FILES}
)

target_link_libraries(@name@-core PUBLIC
    nova::nova
)

add_test_target(@name@-core)
