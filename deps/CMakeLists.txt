add_library(chaiscript INTERFACE)
target_include_directories(chaiscript SYSTEM INTERFACE chai/include)

add_subdirectory(nova/include)
