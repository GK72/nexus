cmake_minimum_required(VERSION 3.24)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

set(NEXUS_VERSION "0.3.9")
project("nexus" VERSION ${NEXUS_VERSION} LANGUAGES CXX)
message(STATUS "Nexus version ${NEXUS_VERSION}")

find_package(nova REQUIRED)
include(GNUInstallDirs)
include(dependencies.cmake)

include(novaCompilerWarnings)
include(novaCompilerSettings)

include(settings.cmake)

include(cmake/gitVersion.cmake)
include(cmake/test.cmake)

include(CTest)

set(NEXUS_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory(libbtx)
add_subdirectory(liblexy)
add_subdirectory(libnxs)

add_subdirectory(baldr)
add_subdirectory(btx)
add_subdirectory(msg-gen)

if(BUILD_TESTING)
    # Root tests if any
endif()
