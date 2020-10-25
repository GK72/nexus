add_library(ui SHARED
    ui.cpp              ui.h
                        action.h
    elements.cpp        elements.h
    input.cpp           input.h
    menu.cpp            menu.h
    message.cpp         message.h
)

target_link_libraries(ui PUBLIC -lncursesw)
target_link_libraries(ui PUBLIC utility)
target_include_directories(ui PRIVATE ../ncx)
