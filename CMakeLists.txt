cmake_minimum_required(VERSION 3.10)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("gLib")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory("console")
add_subdirectory("gGfx")
add_subdirectory("glib")
add_subdirectory("rpg")
add_subdirectory("rts")
add_subdirectory("gclib")

#enable_testing()
