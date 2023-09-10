add_library(chaiscript INTERFACE)
target_include_directories(chaiscript SYSTEM INTERFACE chai/include)

include(yui/cmake/dependencies.cmake)

add_subdirectory(nova/include)
add_subdirectory(yui/include)
add_subdirectory(yui/src)
