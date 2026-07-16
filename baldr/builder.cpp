#include <baldr/builder.hpp>
#include <baldr/command.hpp>

#include <libnxs/line_reader.hpp>
#include <libnxs/rlog.hpp>
#include <libnova/error.hpp>
#include <libnova/log.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr auto DefinesMarkerFile = ".baldr-defines";

constexpr std::array<std::string_view, 4> KnownBuildTypes = {
    "Debug", "Release", "RelWithDebInfo", "MinSizeRel",
};

/**
 * @brief   Validate `build_type` (case-sensitively) against the standard
 *          CMake build types and return its canonical casing.
 *
 * @throws  nova::exception if `build_type` doesn't match any known type.
 */
[[nodiscard]] auto canonical_build_type(const std::string& build_type) -> std::string {
    for (auto known: KnownBuildTypes) {
        if (build_type == known) {
            return std::string(known);
        }
    }

    throw nova::exception("Unknown build type '{}' (expected one of: Debug, Release, RelWithDebInfo, MinSizeRel).", build_type);
}

/**
 * @brief   Lowercase version of `build_type`, used as the build directory
 *          name.
 */
[[nodiscard]] auto lower_build_type(const std::string& build_type) -> std::string {
    auto lower = build_type;
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower;
}

/**
 * @brief   Run `cmd` inside `working_directory`, streaming its combined
 *          stdout/stderr output via `nova::log::info`.
 *
 * @return  The command's exit code.
 */
[[nodiscard]] auto run_streamed(const std::vector<std::string>& args, const std::string& working_directory) -> int {
    auto cmd = baldr::command{ args, {}, working_directory };
    cmd.run();

    nxs::line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), not chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();

    return cmd.wait();
}

/**
 * @brief   Relative path (from `project_dir`) of the CMake build directory
 *          for a given `build_type`.
 */
[[nodiscard]] auto cmake_build_dir(const std::string& build_type) -> std::string {
    return fmt::format("build/{}", lower_build_type(build_type));
}

/**
 * @brief   Keep `<project_dir>/compile_commands.json` symlinked to the
 *          `debug` build's copy, so tooling (e.g. clangd) picks it up from
 *          the repository root without needing to track other build types.
 *
 * A no-op for anything other than the `Debug` build type.
 */
void link_compile_commands(const std::string& project_dir, const std::string& build_type, const fs::path& build_dir) {
    if (build_type != "Debug") {
        return;
    }

    auto link_path = fs::path(project_dir) / "compile_commands.json";
    auto target_path = build_dir / "compile_commands.json";

    if (fs::is_symlink(link_path)) {
        nova::log::debug("Symlink 'compile_commands.json' already exists");
        std::error_code ec;
        auto current_target = fs::read_symlink(link_path, ec);
        if (not ec and current_target == target_path) {
            nova::log::debug("Symlink 'compile_commands.json' is correct");
            return;
        }
    }

    if (not fs::exists(link_path)) {
        std::error_code ec;
        fs::remove(link_path, ec);
        fs::create_symlink(target_path, link_path, ec);
        nova::log::debug("Symlink 'compile_commands.json' has been created");
        if (ec) {
            nova::log::warn("Failed to symlink 'compile_commands.json': {}", ec.message());
        }
    }
}

/**
 * @brief   Serialize `defines` into the same textual form written to /
 *          read from the `.baldr-defines` marker file, so that comparing
 *          two `std::string`s is enough to detect a change.
 */
[[nodiscard]] auto serialize_defines(const std::map<std::string, std::string>& defines) -> std::string {
    std::ostringstream out;
    for (const auto& [key, value]: defines) {
        out << key << '=' << value << '\n';
    }
    return out.str();
}

/**
 * @brief   Decide whether `make` or `cmake` should be used to build the
 *          project in `project_dir`.
 *
 * Automatically (re-)configures CMake if needed: either the build directory
 * was never configured (missing marker file), `CMakeCache.txt` itself is
 * missing (partially-created build directory), the resolved `-D` defines
 * changed since the last configure, or `clean_build` was given.
 *
 * @param   clean_build     If `true`, wipe the resolved build directory
 *                          (CMake) or run `make clean` (Makefile, if a
 *                          `clean` target exists) before building.
 * @param   cmake_defines   `-D` defines to pass to `cmake` at configure
 *                          time; a change from the marker file's recorded
 *                          set of defines triggers a reconfigure.
 *
 * @returns the build command to run.
 */
[[nodiscard]] auto discover_project_type(const std::string& project_dir, const std::string& build_type, bool clean_build, const std::map<std::string, std::string>& cmake_defines) -> std::vector<std::string> {
    if (not fs::exists(fs::path(project_dir) / "CMakeLists.txt")) {
        if (clean_build and fs::exists(fs::path(project_dir) / "Makefile")) {
            nova::log::debug("Cleaning Makefile project in '{}'...", project_dir);
            std::ignore = run_streamed({ "make", "clean" }, project_dir);
        }
        return { "make" };
    }

    auto build_dir_rel = cmake_build_dir(build_type);
    auto build_dir = fs::path(project_dir) / build_dir_rel;

    if (clean_build and fs::exists(build_dir)) {
        nova::log::debug("Deleting build directory '{}'...", build_dir.string());
        fs::remove_all(build_dir);
    }

    auto resolved_defines = serialize_defines(cmake_defines);

    bool needs_configure =
           not fs::exists(build_dir / "CMakeCache.txt")
        or not fs::exists(build_dir / DefinesMarkerFile);

    if (not needs_configure) {
        std::ifstream marker(build_dir / DefinesMarkerFile);
        std::ostringstream recorded;
        recorded << marker.rdbuf();
        needs_configure = recorded.str() != resolved_defines;
    }

    if (needs_configure) {
        nova::log::debug("Configuring CMake project in '{}'...", project_dir);

        std::vector<std::string> configure_cmd{ "cmake", "-S", ".", "-B", build_dir_rel, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" };
        for (const auto& [key, value]: cmake_defines) {
            configure_cmd.push_back(fmt::format("-D{}={}", key, value));
        }

        if (int code = run_streamed(configure_cmd, project_dir); code != 0) {
            throw nova::exception("CMake configure failed (exit code {}).", code);
        }

        fs::create_directories(build_dir);
        std::ofstream(build_dir / DefinesMarkerFile) << resolved_defines;
    }

    link_compile_commands(project_dir, build_type, build_dir);

    return { "cmake", "--build", build_dir_rel };
}

} // namespace

namespace baldr {

builder::builder(
        std::string project_dir,
        std::string build_type,
        std::map<std::string, std::string> cmake_defines
)
    : m_project_dir(std::move(project_dir))
    , m_build_type(canonical_build_type(build_type))
    , m_cmake_defines(std::move(cmake_defines))
{}

void builder::build(bool clean_build) const {
    nova::log::debug("Building in '{}'...", m_project_dir);

    int code = run_streamed(discover_project_type(m_project_dir, m_build_type, clean_build, m_cmake_defines), m_project_dir);
    if (code == 0) {
        nxs::rlog::success("Build successful.");
    } else {
        throw nova::exception("Build failed (exit code {}).", code);
    }
}

/**
 * @note
 *
 * CMake projects place their build output in a dedicated build directory.
 *
 * Makefile-based projects build directly into `project_dir`.
 */
void builder::run(const std::string& target, const std::vector<std::string>& forwarded_args) const {
    // TODO(feat): Discover the executable. It mirrors the build tree.
    auto build_dir_rel = cmake_build_dir(m_build_type);
    auto cmake_target = fs::path(m_project_dir) / build_dir_rel / target;
    std::string exe_path = fs::exists(cmake_target)
        ? fmt::format("./{}/{}", build_dir_rel, target)
        : fmt::format("./{}", target);

    nova::log::debug("Running '{}' in '{}'...", exe_path, m_project_dir);

    std::vector<std::string> argv{ exe_path };
    argv.insert(argv.end(), forwarded_args.begin(), forwarded_args.end());

    auto cmd = command{ argv, {}, m_project_dir, /*interactive=*/true };
    cmd.run();

    if (int code = cmd.wait(); code != 0) {
        throw nova::exception("'{}' exited with code {}.", exe_path, code);
    }
}

} // namespace baldr
