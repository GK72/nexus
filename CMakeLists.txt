cmake_minimum_required(VERSION 3.10)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("gLib")


if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0)
        set(CMAKE_CXX_STANDARD 17)
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++20")
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_STANDARD 17)
elseif (MSVC)
    set(CMAKE_CXX_STANDARD 17)
endif()

add_library(project_warnings INTERFACE)
include(cmake/compilerWarnings.cmake)
set_project_warnings(project_warnings)

include(cmake/settings.cmake)
include(cmake/doxygen.cmake)
enable_doxygen()

add_subdirectory("src")
add_subdirectory("tests")

enable_testing()
