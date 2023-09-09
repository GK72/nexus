add_library(nexus INTERFACE)
target_link_libraries(nexus INTERFACE project_warnings)

target_include_directories(nexus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_HOME_DIRECTORY}/include>
    $<INSTALL_INTERFACE:include>
)

code_analysis(nexus INTERFACE)
