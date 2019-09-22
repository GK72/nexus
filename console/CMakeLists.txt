add_executable (gLib Console.cpp)

target_link_libraries (gLib LINK_PUBLIC gmath)
target_link_libraries (gLib LINK_PUBLIC gGfx)
target_link_libraries (gLib LINK_PUBLIC utility)
target_link_libraries (gLib LINK_PUBLIC curses)

target_include_directories(gLib PUBLIC ../glib)
target_include_directories(gLib PUBLIC ../gGfx)
target_include_directories(gLib PUBLIC /usr/include)