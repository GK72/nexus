#include <baldr/builder.hpp>
#include <baldr/command.hpp>

#include <libnxs/line_reader.hpp>
#include <libnxs/rlog.hpp>
#include <libnova/error.hpp>
#include <libnova/log.hpp>

#include <fmt/format.h>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr auto CMakeBuildDir = "build";

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
 * @brief   Decide whether `make` or `cmake` should be used to build the
 *          project in `project_dir`.
 *
 * Automatically configures CMake if needed.
 *
 * @returns the build command to run.
 *
 * TODO(feat): Build types: debug, release, etc. goes into their own
 *             directory under `build/`. (applicable for CMake only)
 * TODO(feat): Give a CLI arg to enforce re-configuring (or recognize CMake
 *             variable changes automatically)
 */
[[nodiscard]] auto discover_project_type(const std::string& project_dir) -> std::vector<std::string> {
    if (not fs::exists(fs::path(project_dir) / "CMakeLists.txt")) {
        return { "make" };
    }

    auto build_dir = fs::path(project_dir) / CMakeBuildDir;
    if (not fs::exists(build_dir / "CMakeCache.txt")) {
        nova::log::debug("Configuring CMake project in '{}'...", project_dir);
        if (int code = run_streamed({ "cmake", "-S", ".", "-B", CMakeBuildDir }, project_dir); code != 0) {
            throw nova::exception("CMake configure failed (exit code {}).", code);
        }
    }

    return { "cmake", "--build", CMakeBuildDir };
}

} // namespace

namespace baldr {

builder::builder(std::string project_dir)
    : m_project_dir(std::move(project_dir))
{}

void builder::build() const {
    nova::log::debug("Building in '{}'...", m_project_dir);

    int code = run_streamed(discover_project_type(m_project_dir), m_project_dir);
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
void builder::run(const std::string& target) const {
    // TODO(feat): Discover the executable. It mirrors the build tree.
    auto cmake_target = fs::path(m_project_dir) / CMakeBuildDir / target;
    std::string exe_path = fs::exists(cmake_target)
        ? fmt::format("./{}/{}", CMakeBuildDir, target)
        : fmt::format("./{}", target);

    nova::log::debug("Running '{}' in '{}'...", exe_path, m_project_dir);

    auto cmd = command{ { exe_path }, {}, m_project_dir, /*interactive=*/true };
    cmd.run();

    if (int code = cmd.wait(); code != 0) {
        throw nova::exception("'{}' exited with code {}.", exe_path, code);
    }
}

} // namespace baldr
