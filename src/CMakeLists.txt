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

add_executable(cx2 main.cpp2)
cppfront_enable(TARGETS cx2)

target_link_libraries(cx2 PRIVATE fmt)
