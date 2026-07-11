#include <baldr/utils.hpp>
#include <system_error>

namespace baldr::utils {

void create_compile_commands_symlink(const std::filesystem::path& build_dir, const std::filesystem::path& project_path) {
    auto cc_json = build_dir / "compile_commands.json";
    if (std::filesystem::exists(cc_json)) {
        auto symlink = project_path / "compile_commands.json";
        std::error_code ec;
        if (std::filesystem::exists(symlink)) {
            std::filesystem::remove(symlink, ec);
        }
        std::filesystem::create_symlink(cc_json, symlink, ec);
    }
}

auto find_executables(const std::filesystem::path& dir) -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> executables;
    if (!std::filesystem::exists(dir)) return executables;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto perms = entry.status().permissions();
            if ((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
                (perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ||
                (perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none) {
                executables.push_back(entry.path());
            }
        }
    }
    return executables;
}

} // namespace baldr::utils
