add_executable(baldr main.cpp builder.cpp docker.cpp)
target_link_libraries(baldr PRIVATE nxs Boost::program_options Boost::headers nlohmann_json::nlohmann_json)

nexus_git_hash(BALDR_GIT_HASH)

target_compile_definitions(baldr PRIVATE
    BALDR_VERSION="${NEXUS_VERSION}"
    BALDR_GIT_HASH="${BALDR_GIT_HASH}"
)

install(TARGETS baldr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(example-command command.ex.cpp)
target_link_libraries(example-command PRIVATE nxs)
