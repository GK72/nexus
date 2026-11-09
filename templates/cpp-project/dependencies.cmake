# Versions are pinned in conandata.yml; tools/configure.sh installs them.
# GTest is found by cmake/test.cmake, only when the tests are built.
find_package(nova REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
