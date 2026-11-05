file(GLOB_RECURSE FOLD_HEADER_FILES "*.hpp")
list(FILTER FOLD_HEADER_FILES EXCLUDE REGEX "\\.test\\.hpp$")

add_library(fold INTERFACE)

target_sources(fold INTERFACE
    FILE_SET HEADERS
    BASE_DIRS "${NEXUS_PROJECT_ROOT}"
    FILES "${FOLD_HEADER_FILES}"
)

target_include_directories(fold INTERFACE
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

add_test_target(fold)

add_executable(example-fold fold.ex.cpp)
target_link_libraries(example-fold PRIVATE fold)

install(TARGETS fold
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
