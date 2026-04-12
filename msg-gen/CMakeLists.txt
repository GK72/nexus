add_executable(msg-gen main.cpp)
target_link_libraries(msg-gen PRIVATE btx Boost::program_options)

install(TARGETS msg-gen RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
