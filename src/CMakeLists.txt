add_library(nexus
    ph.cc
)

target_include_directories(nexus PUBLIC
    $<BUILD_INTERFACE:${CMAKE_HOME_DIRECTORY}/include>
    $<INSTALL_INTERFACE:include>
    ${CONAN_INCLUDE_DIRS}
)

target_link_libraries(nexus PUBLIC
    ${CONAN_LIBS}
)
