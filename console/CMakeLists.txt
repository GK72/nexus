add_executable (gLib Console.cpp)

target_link_libraries (gLib	LINK_PUBLIC rpg)
target_link_libraries (gLib	LINK_PUBLIC rts)
target_link_libraries (gLib	LINK_PUBLIC gmath)
target_link_libraries (gLib	LINK_PUBLIC utility)
target_link_libraries (gLib	LINK_PUBLIC gGfx)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_link_libraries (gLib LINK_PUBLIC curses)
	target_include_directories(/usr/include)
endif()

target_include_directories(gLib PUBLIC ../glib)
target_include_directories(gLib PUBLIC ../rpg)
target_include_directories(gLib PUBLIC ../rts)
target_include_directories(gLib PUBLIC ../gGfx)


add_executable (measure measuring.cpp)
target_link_libraries (measure LINK_PUBLIC utility)
target_include_directories(measure PUBLIC ../glib)
