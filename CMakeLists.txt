cmake_minimum_required(VERSION 3.10)

project("gLib")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++17")
	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
endif()

add_subdirectory("console")
add_subdirectory("gGfx")
add_subdirectory("glib")
add_subdirectory("rpg")