add_executable(example main.cc)
target_link_libraries(example PUBLIC ${CONAN_LIBS})

# TODO: link `project_warnings` from `nexus`
