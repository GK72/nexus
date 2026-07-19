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
[[nodiscard]] auto run_streamed(
        const std::vector<std::string>& args,
        const std::string& working_directory,
        const std::map<std::string, std::string>& env = {}
) -> int {
    auto cmd = baldr::command{ args, env, working_directory };
    cmd.run();

    nxs::line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), not chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();

    return cmd.wait().code();
}

/**
 * @brief   Relative path (from `project_dir`) of the CMake build directory
 *          for a given `build_type`.
 */
[[nodiscard]] auto cmake_build_dir(const std::string& build_type) -> std::string {
    return fmt::format("build/{}", lower_build_type(build_type));
}

/**
 * @brief   Search `build_dir` recursively for a regular, executable file
 *          named `target`
 *
 * @return  The paths to all matching executables.
 *
 * @note    Multiple executables can be found if the project structure is refactored.
 */
[[nodiscard]] auto
find_built_executables(const fs::path& build_dir, const std::string& target) -> std::vector<fs::path> {
    std::vector<fs::path> ret;

    if (not fs::exists(build_dir)) {
        throw nova::exception("Build directory does not exist: `{}`", build_dir.string());
    }

    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(build_dir, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator();
         it.increment(ec)
    ) {
        if (ec) {
            break;
        }

        const auto& entry = *it;
        if (entry.path().filename() != target) {
            continue;
        }

        if (not entry.is_regular_file()) {
            continue;
        }

        bool is_executable =
            (entry.status().permissions() &
                (fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec)
            ) != fs::perms::none;

        if (is_executable) {
            ret.push_back(std::move(entry.path()));
        }
    }

    return ret;
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
 * @brief   Serialize `defines` and `env` into the same textual form written
 *          to / read from the `.baldr-defines` marker file, so that
 *          comparing two `std::string`s is enough to detect a change (either
 *          one can affect the configured build).
 */
[[nodiscard]] auto serialize_defines(
        const std::map<std::string, std::string>& defines,
        const std::map<std::string, std::string>& env
) -> std::string {
    std::ostringstream out;
    for (const auto& [key, value]: defines) {
        out << key << '=' << value << '\n';
    }
    out << "--env--\n";
    for (const auto& [key, value]: env) {
        out << key << '=' << value << '\n';
    }
    return out.str();
}

} // namespace

namespace baldr {

builder::builder(
        std::string project_dir,
        std::string build_type,
        std::map<std::string, std::string> cmake_defines,
        std::map<std::string, std::string> cmake_env,
        std::string debugger,
        std::vector<std::string> debugger_args
)
    : m_project_dir(std::move(project_dir))
    , m_build_type(canonical_build_type(build_type))
    , m_cmake_defines(std::move(cmake_defines))
    , m_cmake_env(std::move(cmake_env))
    , m_debugger(std::move(debugger))
    , m_debugger_args(std::move(debugger_args))
{}

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
[[nodiscard]] auto
builder::discover_project_type(bool clean_build) -> std::vector<std::string> {
    if (not fs::exists(fs::path(m_project_dir) / "CMakeLists.txt")) {
        nova::log::debug("Discovered Makefile project in '{}'", m_project_dir);

        if (clean_build and fs::exists(fs::path(m_project_dir) / "Makefile")) {
            nova::log::debug("Cleaning Makefile project in '{}'...", m_project_dir);
            std::ignore = run_streamed({ "make", "clean" }, m_project_dir);
        }
        m_project_type = project_type::make;
        return { "make" };
    }

    nova::log::debug("Discovered CMake project in '{}'", m_project_dir);
    m_project_type = project_type::cmake;

    auto build_dir_rel = cmake_build_dir(m_build_type);
    auto build_dir = fs::path(m_project_dir) / build_dir_rel;

    if (clean_build and fs::exists(build_dir)) {
        nova::log::debug("Deleting build directory '{}'...", build_dir.string());
        fs::remove_all(build_dir);
    }

    auto resolved_defines = serialize_defines(m_cmake_defines, m_cmake_env);

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
        nova::log::debug("Configuring CMake project in '{}'...", m_project_dir);

        std::vector<std::string> configure_cmd{ "cmake", "-S", ".", "-B", build_dir_rel, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" };
        for (const auto& [key, value]: m_cmake_defines) {
            configure_cmd.push_back(fmt::format("-D{}={}", key, value));
        }

        if (int code = run_streamed(configure_cmd, m_project_dir, m_cmake_env); code != 0) {
            throw nova::exception("CMake configure failed (exit code {}).", code);
        }

        fs::create_directories(build_dir);
        std::ofstream(build_dir / DefinesMarkerFile) << resolved_defines;
    }

    link_compile_commands(m_project_dir, m_build_type, build_dir);

    return { "cmake", "--build", build_dir_rel };
}

void builder::build(bool clean_build) {
    nova::log::debug("Building in '{}'...", m_project_dir);

    int code = run_streamed(discover_project_type(clean_build), m_project_dir, m_cmake_env);
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
void builder::run(const std::string& target, const std::vector<std::string>& forwarded_args, bool debug) {
    const auto build_dir = fs::path(m_project_dir) / cmake_build_dir(m_build_type);

    std::string exe_path;

    switch (m_project_type) {
        case project_type::make: {
            exe_path = fs::path(m_project_dir) / target;
            break;
        }
        case project_type::cmake: {
            auto exes = find_built_executables(build_dir, target);
            if (exes.empty()) {
                throw nova::exception("No executable found for target `{}`", target);
            }

            if (exes.size() > 1) {
                throw nova::exception("Multiple executables found for target `{}` (suggested to make a clean build)", target);
            }

            exe_path = fmt::format("./{}", fs::relative(exes[0], m_project_dir).string());
            break;
        }
        default:
            throw nova::exception("Unsupported project type");
    }

    std::vector<std::string> argv;
    if (debug) {
        argv.push_back(m_debugger);
        argv.insert(argv.end(), m_debugger_args.begin(), m_debugger_args.end());
        nova::log::debug("Running '{}' in '{}' via debugger...", exe_path, m_project_dir);
    } else {
        nova::log::debug("Running '{}' in '{}'...", exe_path, m_project_dir);
    }
    argv.push_back(exe_path);
    argv.insert(argv.end(), forwarded_args.begin(), forwarded_args.end());

    auto cmd = command{ argv, m_cmake_env, m_project_dir, /*interactive=*/true };
    cmd.run();

    if (auto status = cmd.wait(); not status.success()) {
        throw nova::exception("{}", status.describe());
    }
}

} // namespace baldr
