add_library(chai-bindings chai_bindings.cc)
target_link_libraries(chai-bindings PUBLIC chaiscript)

add_executable(nexus-app main.cc)

target_link_libraries(nexus-app PRIVATE
    chai-bindings
    nexus
    nova
    fmt
)
