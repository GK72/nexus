#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace baldr::utils {

/**
 * @brief Create a symlink to compile_commands.json in the project root.
 * 
 * @param build_dir Path to the build directory.
 * @param project_path Path to the project root.
 */
void create_compile_commands_symlink(const std::filesystem::path& build_dir, const std::filesystem::path& project_path);

/**
 * @brief Recursively find all executable files in a directory.
 * 
 * @param dir The directory to search.
 * @returns A vector of paths to executable files.
 */
auto find_executables(const std::filesystem::path& dir) -> std::vector<std::filesystem::path>;

} // namespace baldr::utils
