#include <baldr/builder.hpp>
#include <baldr/command.hpp>

#include <libnxs/line_reader.hpp>
#include <libnxs/rlog.hpp>
#include <libnova/error.hpp>
#include <libnova/log.hpp>
#include <libnova/utils.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
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

    throw nova::exception("Unknown build type `{}` (expected one of: Debug, Release, RelWithDebInfo, MinSizeRel).", build_type);
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
            nova::log::debug("Found executable: `{}`", entry.path().string());
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
        nova::log::debug("Symlink `compile_commands.json` already exists");
        std::error_code ec;
        auto current_target = fs::read_symlink(link_path, ec);
        if (not ec and current_target == target_path) {
            nova::log::debug("Symlink `compile_commands.json` is correct");
            return;
        }
    }

    if (not fs::exists(link_path)) {
        std::error_code ec;
        fs::remove(link_path, ec);
        fs::create_symlink(target_path, link_path, ec);
        nova::log::debug("Symlink `compile_commands.json` has been created");
        if (ec) {
            nova::log::warn("Failed to symlink `compile_commands.json`: {}", ec.message());
        }
    }
}

/**
 * @brief   Serialize `defines`, `env` and the resolved Conan provider path
 *          (if any) into the same textual form written to / read from the
 *          `.baldr-defines` marker file, so that comparing two
 *          `std::string`s is enough to detect a change (either one can
 *          affect the configured build).
 */
[[nodiscard]] auto serialize_defines(
        const std::map<std::string, std::string>& defines,
        const std::map<std::string, std::string>& env,
        const std::optional<std::string>& conan_provider = std::nullopt
) -> std::string {
    std::ostringstream out;

    for (const auto& [key, value]: defines) {
        out << key << '=' << value << '\n';
    }

    out << "--env--\n";

    for (const auto& [key, value]: env) {
        out << key << '=' << value << '\n';
    }

    out << "--conan--\n";
    out << conan_provider.value_or("") << '\n';

    return out.str();
}

/**
 * @brief   Whether `build_dir` needs a (re-)configure: missing cache,
 *          missing marker file, or a marker mismatch against
 *          `resolved_defines`.
 */
[[nodiscard]] auto
needs_cmake_configure(const fs::path& build_dir, const std::string& resolved_defines) -> bool {
    bool needs_configure =
           not fs::exists(build_dir / "CMakeCache.txt")
        || not fs::exists(build_dir / DefinesMarkerFile);

    if (not needs_configure) {
        std::ifstream marker(build_dir / DefinesMarkerFile);
        std::ostringstream recorded;
        recorded << marker.rdbuf();
        needs_configure = recorded.str() != resolved_defines;
    }

    return needs_configure;
}

} // namespace

namespace baldr {

builder::builder(
        std::string project_dir,
        config cfg
)
    : m_project_dir(std::move(project_dir))
    , m_build_type(canonical_build_type(cfg.build_type))
    , m_cmake_defines(std::move(cfg.cmake_defines))
    , m_cmake_env(std::move(cfg.env))
    , m_debugger(std::move(cfg.debugger))
    , m_debugger_args(std::move(cfg.debugger_args))
{}

/**
 * @brief   Handle the Makefile-project branch of
 *          `discover_project_type`: optionally run `make clean`, then
 *          return the build command.
 */
[[nodiscard]] auto
builder::handle_makefile_project(bool clean_build) const -> std::vector<std::string> {
    nova::log::debug("Discovered Makefile project in `{}`", m_project_dir);

    if (clean_build and fs::exists(fs::path(m_project_dir) / "Makefile")) {
        nova::log::debug("Cleaning Makefile project in `{}`...", m_project_dir);
        std::ignore = run_streamed({ "make", "clean" }, m_project_dir);
    }

    return { "make" };
}

/**
 * @brief   Run `cmake`'s configure step for `build_dir_rel`, then
 *          record `resolved_defines` in the marker file.
 *
 * @throws  nova::exception if the configure command fails.
 */
void builder::configure_cmake(
        const fs::path& build_dir,
        const std::string& build_dir_rel,
        const std::string& resolved_defines,
        const std::optional<std::string>& conan_provider
) const {
    nova::log::debug("Configuring CMake project in `{}`...", m_project_dir);

    auto configure_cmd = std::vector<std::string>{
        "cmake",
        "-S", ".",
        "-B", build_dir_rel,
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    };

    for (const auto& [key, value]: m_cmake_defines) {
        configure_cmd.push_back(fmt::format("-D{}={}", key, value));
    }

    if (conan_provider) {
        nova::log::debug("Discovered Conan project, injecting CMAKE_PROJECT_TOP_LEVEL_INCLUDES=`{}`", *conan_provider);
        configure_cmd.push_back(fmt::format("-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES={}", *conan_provider));
    }

    if (int code = run_streamed(configure_cmd, m_project_dir, m_cmake_env); code != 0) {
        throw nova::exception("CMake configure failed (exit code {}).", code);
    }

    fs::create_directories(build_dir);
    std::ofstream(build_dir / DefinesMarkerFile) << resolved_defines;
}

/**
 * @brief   Resolve the `conan_provider.cmake` to inject via
 *          `CMAKE_PROJECT_TOP_LEVEL_INCLUDES`, if `m_project_dir` looks like
 *          a Conan-managed project.
 *
 * Only applies when a `conanfile.txt`/`conanfile.py` exists in
 * `m_project_dir` and the user hasn't already supplied
 * `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` themselves; prefers a project-local
 * `conan_provider.cmake`, falling back to `$HOME/conan_provider.cmake`.
 *
 * @return  The resolved provider path, or `std::nullopt` if Conan
 *          auto-discovery doesn't apply.
 */
[[nodiscard]] auto
builder::resolve_conan_provider() const -> std::optional<std::string> {
    if (m_cmake_defines.contains("CMAKE_PROJECT_TOP_LEVEL_INCLUDES")) {
        nova::log::debug("CMAKE_PROJECT_TOP_LEVEL_INCLUDES was defined, skipping Conan auto-configure");
        return std::nullopt;
    }

    const auto project_path = fs::path(m_project_dir);
    bool has_conanfile = fs::exists(project_path / "conanfile.txt")
        || fs::exists(project_path / "conanfile.py");

    if (not has_conanfile) {
        nova::log::debug("No conanfile has been found");
        return std::nullopt;
    }

    if (auto project_local = project_path / "conan_provider.cmake"; fs::exists(project_local)) {
        nova::log::debug("Using project provided `conan_provider.cmake`");
        return project_local.string();
    }

    auto home = nova::getenv("HOME").value();
    if (auto home_provider = fs::path(home) / "conan_provider.cmake"; fs::exists(home_provider)) {
        nova::log::debug("Using globally provided `~/conan_provider.cmake`");
        return home_provider.string();
    }

    nova::log::warn("No `conan_provider.cmake` has been found");

    return std::nullopt;
}

[[nodiscard]] auto
builder::discover_project_type(bool clean_build) -> std::vector<std::string> {
    if (not fs::exists(fs::path(m_project_dir) / "CMakeLists.txt")) {
        m_project_type = project_type::make;
        return handle_makefile_project(clean_build);
    }

    nova::log::debug("Discovered CMake project in `{}`", m_project_dir);
    m_project_type = project_type::cmake;

    auto build_dir_rel = cmake_build_dir(m_build_type);
    auto build_dir = fs::path(m_project_dir) / build_dir_rel;

    if (clean_build && fs::exists(build_dir)) {
        nova::log::debug("Deleting build directory `{}`...", build_dir.string());
        fs::remove_all(build_dir);
    }

    auto conan_provider = resolve_conan_provider();
    auto resolved_defines = serialize_defines(m_cmake_defines, m_cmake_env, conan_provider);

    if (needs_cmake_configure(build_dir, resolved_defines)) {
        configure_cmake(build_dir, build_dir_rel, resolved_defines, conan_provider);
    }

    link_compile_commands(m_project_dir, m_build_type, build_dir);

    return { "cmake", "--build", build_dir_rel };
}

void builder::build(bool clean_build) {
    nova::log::debug("Building in `{}`...", m_project_dir);

    int code = run_streamed(discover_project_type(clean_build), m_project_dir, m_cmake_env);
    if (code == 0) {
        nxs::rlog::success("Build successful.");
    } else {
        throw nova::exception("Build failed (exit code {}).", code);
    }
}

/**
 * @brief   Resolve `target`'s executable path, according to
 *          `m_project_type` (an exact `Makefile` path, or the unique
 *          match found by searching the CMake build directory).
 *
 * @throws  nova::exception if `target` can't be found or is ambiguous
 *          (CMake projects only).
 */
[[nodiscard]] auto
builder::resolve_executable(const std::string& target) const -> std::string {
    switch (m_project_type) {
        case project_type::make: {
            return fs::path(m_project_dir) / target;
        }
        case project_type::cmake: {
            const auto build_dir = fs::path(m_project_dir) / cmake_build_dir(m_build_type);
            auto exes = find_built_executables(build_dir, target);
            if (exes.empty()) {
                throw nova::exception("No executable found for target `{}`", target);
            }

            if (exes.size() > 1) {
                throw nova::exception("Multiple executables found for target `{}` (suggested to make a clean build)", target);
            }

            return fmt::format("./{}", fs::relative(exes[0], m_project_dir).string());
        }
        default:
            throw nova::exception("Unsupported project type");
    }
}

/**
 * @brief   Build the `argv` for launching `exe_path`, optionally
 *          prefixed with the configured debugger.
 *
 * @param   exe_path        Resolved path to the executable to run.
 * @param   forwarded_args  Extra arguments appended after `exe_path`.
 * @param   debug           If `true`, prepend `m_debugger`/
 *                          `m_debugger_args` ahead of `exe_path`.
 */
[[nodiscard]] auto builder::build_argv(
        const std::string& exe_path,
        const std::vector<std::string>& forwarded_args,
        bool debug
) const -> std::vector<std::string> {
    std::vector<std::string> argv;

    if (debug) {
        argv.push_back(m_debugger);
        argv.insert(argv.end(), m_debugger_args.begin(), m_debugger_args.end());
    }

    argv.push_back(exe_path);
    argv.insert(argv.end(), forwarded_args.begin(), forwarded_args.end());

    return argv;
}

void builder::run(const std::string& target, const std::vector<std::string>& forwarded_args, bool debug) {
    auto exe_path = resolve_executable(target);

    if (debug) {
        nova::log::debug("Running `{}` in `{}` via debugger...", exe_path, m_project_dir);
    } else {
        nova::log::debug("Running `{}` in `{}`...", exe_path, m_project_dir);
    }

    auto argv = build_argv(exe_path, forwarded_args, debug);

    auto cmd = command{ argv, m_cmake_env, m_project_dir, /*interactive=*/true };
    cmd.run();

    if (auto status = cmd.wait(); not status.success()) {
        throw nova::exception("{}", status.describe());
    }
}

} // namespace baldr
