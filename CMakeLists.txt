cmake_minimum_required(VERSION 3.10)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("gLib")

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
	add_compile_options(/std:c++17)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++17")
	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
endif()

add_subdirectory("console")
add_subdirectory("gGfx")
add_subdirectory("glib")
add_subdirectory("rpg")
add_subdirectory("rts")


enable_testing()
