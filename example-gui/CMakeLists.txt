# Conan import
add_library(imguiBackendGL
    ${CMAKE_BINARY_DIR}/bindings/imgui_impl_glfw.cpp
    ${CMAKE_BINARY_DIR}/bindings/imgui_impl_opengl3.cpp
)

add_executable(example-gui
    main.cc
)

target_link_libraries(example-gui PUBLIC
    nexus
    imguiBackendGL
    ${CONAN_LIBS}
)

target_include_directories(example-gui PRIVATE
    ${CMAKE_BINARY_DIR}
)

if (UNIX)
    target_link_libraries(example-gui PRIVATE
        pthread
        GL
    )
endif()

code_analysis(example-gui PRIVATE)

add_custom_command(
    TARGET example-gui
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
        ${CMAKE_BINARY_DIR}/bin/res
)
