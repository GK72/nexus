add_executable(example main.cc)
target_link_libraries(example PUBLIC ${CONAN_LIBS})

code_analysis(example PRIVATE)

install(TARGETS example)

# Conan copies the necessary shared libraries to the `lib` directory in `build`
# as configured in `conanfile.txt`
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/ DESTINATION lib)
