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

target_sources(nexus
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES
    FILES
    ph.cxx
)

code_analysis(nexus PRIVATE)

add_executable(main main.cc)
target_link_libraries(main nexus)
