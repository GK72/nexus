add_executable (rpg main.cpp)

target_link_libraries (rpg LINK_PUBLIC gGfx)
target_link_libraries (rpg LINK_PUBLIC curses)

target_include_directories(rpg PUBLIC ../gGfx)
target_include_directories(rpg PUBLIC /usr/include)