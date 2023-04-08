add_library(nexus
    gui.cc
    # ph.cc
)

target_include_directories(nexus PUBLIC
    $<BUILD_INTERFACE:${CMAKE_HOME_DIRECTORY}/include>
    $<INSTALL_INTERFACE:include>
    ${CONAN_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(nexus PUBLIC
    ${CONAN_LIBS}
)

code_analysis(nexus PRIVATE)
