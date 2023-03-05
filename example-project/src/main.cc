/**
 * @brief   An example project how to use a Conan packaged library
 *
 * Importing headers, C++ libraries
 *
 * TODO: CMake library import for compiler settings, static analysis, coverage,
 * etc...
 */

#include <nxs/data.h>

#include <string_view>

int main() {
    using namespace std::literals::string_view_literals;
    spdlog::info(nxs::data_view("Hello Nexus"sv).to_hex());
}
