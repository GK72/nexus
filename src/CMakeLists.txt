add_library(chai-bindings chai_bindings.cc)
target_link_libraries(chai-bindings PUBLIC chaiscript)

add_executable(pt-app
    pt_app.cc
)

target_link_libraries(pt-app PRIVATE
    # chai-bindings
    pt
    yui
)

add_subdirectory(pt)
