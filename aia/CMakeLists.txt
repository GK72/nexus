add_executable(aia
    main.cpp
)

target_link_libraries(aia PRIVATE
    nova::nova
    llama-cpp::llama-cpp
    Boost::program_options
    nlohmann_json::nlohmann_json
)

install(TARGETS aia RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
