add_library(pt pathtracer.cc)
target_link_libraries(pt PUBLIC
    nova
    fmt
)
