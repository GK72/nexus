cmake_minimum_required(VERSION 3.28)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/version.txt" @NAME@_VERSION LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version.txt)
project(@name@ VERSION ${@NAME@_VERSION} LANGUAGES CXX)
message(STATUS "@name@ version ${@NAME@_VERSION}")

set(@NAME@_ROOT "${CMAKE_CURRENT_LIST_DIR}")

include(GNUInstallDirs)
include(dependencies.cmake)
include(settings.cmake)
include(cmake/test.cmake)

add_subdirectory(lib@name@)
add_subdirectory(@name@)
