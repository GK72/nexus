add_executable(bench-math math.cc)
target_link_libraries(bench-math PRIVATE nova benchmark)
