# add_library(chai-bindings chai_bindings.cc)
# target_link_libraries(chai-bindings PUBLIC chaiscript)

add_executable(nexus-gui
    gui.cc
    pathtracer.cc
)

target_link_libraries(nexus-gui PRIVATE
    # chai-bindings
    nexus

    nova
    yui
    fmt
)

add_executable(pt
    pt-main.cc
    pathtracer.cc
)

target_link_libraries(pt PRIVATE
    nexus
    nova
    yui
)
