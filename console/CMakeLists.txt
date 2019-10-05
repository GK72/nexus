add_executable (gLib Console.cpp)

target_link_libraries (gLib LINK_PUBLIC rpg)
target_link_libraries (gLib LINK_PUBLIC gmath)
target_link_libraries (gLib LINK_PUBLIC gGfx)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries (gLib LINK_PUBLIC curses)
endif()

target_include_directories(gLib PUBLIC ../glib)
target_include_directories(gLib PUBLIC ../rpg)
target_include_directories(gLib PUBLIC ../gGfx)
target_include_directories(gLib PUBLIC /usr/include)
