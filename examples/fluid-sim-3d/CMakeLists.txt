add_executable(fluid-sim-3d main.cpp)
target_link_libraries(fluid-sim-3d PRIVATE raylib)

install(TARGETS fluid-sim-3d RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
