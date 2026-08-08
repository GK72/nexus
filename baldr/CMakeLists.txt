file(GLOB_RECURSE BALDR_HEADER_FILES "*.hpp")

add_library(builder
    builder.cpp
    docker.cpp
    config.cpp
)

target_link_libraries(builder PUBLIC nxs Boost::headers)
add_test_target(builder)

target_sources(builder PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${NEXUS_PROJECT_ROOT}"
    FILES "${BALDR_HEADER_FILES}"
)

target_include_directories(builder PUBLIC
    $<BUILD_INTERFACE:${NEXUS_PROJECT_ROOT}>
    $<INSTALL_INTERFACE:include>
)

add_executable(baldr main.cpp)
target_link_libraries(baldr PRIVATE builder Boost::program_options)

nexus_git_hash(BALDR_GIT_HASH)
nexus_git_branch(BALDR_GIT_BRANCH)
nexus_git_dirty(BALDR_GIT_DIRTY)

target_compile_definitions(baldr PRIVATE
    BALDR_VERSION="${NEXUS_VERSION}"
    BALDR_GIT_HASH="${BALDR_GIT_HASH}"
    BALDR_GIT_BRANCH="${BALDR_GIT_BRANCH}"
    BALDR_GIT_DIRTY="${BALDR_GIT_DIRTY}"
)

install(TARGETS baldr COMPONENT baldr)

add_executable(example-command command.ex.cpp)
target_link_libraries(example-command PRIVATE builder)
