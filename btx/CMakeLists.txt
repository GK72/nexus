add_executable(btx-tool main.cpp)
target_link_libraries(btx-tool PRIVATE btx lexy Boost::program_options)

install(TARGETS btx-tool RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
