#include <baldr/builder.hpp>
#include <baldr/command.hpp>
#include <baldr/line_reader.hpp>
#include <baldr/utils.hpp>

#include <libnova/log.hpp>

#include <fmt/ranges.h>

#include <filesystem>

namespace baldr {

Builder::Builder(const Config& config)
    : m_config(config)
{}

auto Builder::configure(const std::string& project_path, const std::string& build_dir) -> int {
    nova::log::info("Configuring project...");

    std::vector<std::string> args = {
        "cmake",
        "-S", project_path,
        "-B", build_dir,
        "-DCMAKE_BUILD_TYPE=" + m_config.build_type,
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    };

    for (const auto& arg : m_config.cmake_args) {
        args.push_back(arg);
    }

    std::map<std::string, std::string> env;
    if (m_config.cc) {
        env["CC"] = *m_config.cc;
    }
    if (m_config.cxx) {
        env["CXX"] = *m_config.cxx;
    }

    command cmd(args, env);
    cmd.run();

    line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), !chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();

    int code = cmd.wait();
    if (code == 0) {
        utils::create_compile_commands_symlink(build_dir, project_path);
        nova::log::info("Configuration successful");
    } else {
        nova::log::error("Configuration failed");
    }
    return code;
}

auto Builder::build(const std::string& build_dir, const std::string& target) -> int {
    nova::log::info("Building target {}...", target);

    const auto cache = std::filesystem::path(build_dir) / "CMakeCache.txt";
    nova::log::debug(
        "build: build_dir='{}' (exists={}), CMakeCache.txt present={}, target='{}', jobs={}",
        build_dir,
        std::filesystem::exists(build_dir),
        std::filesystem::exists(cache),
        target,
        m_config.jobs
    );

    std::vector<std::string> args = {
        "cmake",
        "--build", build_dir,
        "--target", target,
        "--",
        "-j", std::to_string(m_config.jobs)
    };

    nova::log::debug("build: executing: {}", fmt::join(args, " "));

    command cmd(args);
    cmd.run();

    line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), !chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();

    int code = cmd.wait();
    if (code == 0) {
        nova::log::info("Build successful");
    } else {
        nova::log::error("Build failed");
    }
    return code;
}

auto Builder::debug(const std::string& build_dir, const std::vector<std::string>& exe_args) -> int {
    auto executables = utils::find_executables(build_dir);
    if (executables.empty()) {
        nova::log::error("No executables found in {}", build_dir);
        return EXIT_FAILURE;
    }

    // Heuristic: pick the first one.
    auto exe = executables[0];
    nova::log::info("Debugging {}...", exe.string());

    std::vector<std::string> args = { "gdb", exe.string() };
    for (const auto& arg : exe_args) {
        args.push_back(arg);
    }

    command cmd(args, {}, true);
    cmd.run();

    return cmd.wait();
}

} // namespace baldr
