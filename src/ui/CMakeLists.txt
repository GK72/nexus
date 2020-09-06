add_library(ui SHARED
    ui.cpp      ui.h
)

target_link_libraries(ui PUBLIC -lncursesw)
target_include_directories(ui PRIVATE ../ncx)
