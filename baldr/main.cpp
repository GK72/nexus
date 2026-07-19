/**
 * Part of Baldr.
 *
 * Baldr CLI entry point.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Sonnet 5 (Junie)
 * @date        2026-07-12
 */

#include <baldr/builder.hpp>
#include <baldr/config.hpp>
#include <baldr/docker.hpp>

#include <libnxs/rlog.hpp>

#include <libnova/error.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>

#include <boost/program_options.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace po = boost::program_options;

namespace {

#ifndef BALDR_VERSION
#define BALDR_VERSION "unknown"
#endif

#ifndef BALDR_GIT_HASH
#define BALDR_GIT_HASH "unknown"
#endif

[[nodiscard]] auto baldr_version() {
    return fmt::format("baldr v{}+{} (hosted by Nexus project)", BALDR_VERSION, BALDR_GIT_HASH);
}

/**
 * @brief   Print the usage/help message to `out`.
 */
void print_help(std::ostream& out) {
    out << "Usage: baldr [options] <command>\n";
    out << "Options:\n";
    out << "  -h, --help                Produce help message\n";
    out << "  -v, --version             Print version and exit\n";
    out << "  -p, --project <dir>       Project directory (default: current directory)\n";
    out << "      --build-type <name>   CMake build type/output subdir (default: 'Debug')\n";
    out << "      --clean               For 'build': wipe the build directory before building (clean build)\n";
    out << "  -D, --cmake-define        CMake define, repeatable (e.g. -DFOO=1); triggers reconfigure on change\n";
    out << "  -t, --target <name>       Executable name to run (required for 'run')\n";
    out << "      --build               For 'run': build the project first\n";
    out << "      --debug               For 'run': launch the target under the configured debugger (default: 'gdb --args')\n";
    out << "      -- <args...>          For 'run': forward everything after '--' to the target's own argv\n";
    out << "  -i, --image <name>        Docker image to use (required for 'docker')\n";
    out << "\n";
    out << "  CMake projects are always configured with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON;\n";
    out << "  for the 'Debug' build type, <project_dir>/compile_commands.json is kept\n";
    out << "  symlinked to it (no need to switch it for other build types).\n";
    out << "\n";
    out << "  A project-local '.baldr.yaml' (falling back to '~/.baldr.yaml') can supply\n";
    out << "  default 'build_type', 'cmake.definitions' and 'cmake.env'; CLI flags always\n";
    out << "  take precedence over config values.\n";
    out << "\n";
    out << "Commands:\n";
    out << "  build      Configure (if needed) and build the project\n";
    out << "  run        Run the built target given via -t/--target\n";
    out << "  docker     Run a command inside a container: baldr docker -i <image> <cmd...>\n";
}

enum class command_type {
    build,
    run,
    docker,
};

/**
 * @brief   Outcome of parsing the CLI arguments.
 *
 * `help` is set whenever the process should exit successfully without
 * running any command (e.g. `--help`/`--version` was given); `command`/
 * `log_mode` are only meaningful otherwise.
 */
struct options {
    command_type command;
    std::string project_dir;
    std::string build_type;
    bool clean_build = false;
    std::map<std::string, std::string> cmake_defines;
    bool build_type_explicit = false;
    std::optional<std::string> target;
    bool build_before_run = false;
    bool debug = false;
    std::optional<std::string> image;
    std::vector<std::string> docker_args;
    std::vector<std::string> forwarded_args;
};

/**
 * @brief   Parse `args` (excluding the program name) into a `options`.
 *
 * Everything following the literal `--` is forwarded verbatim to the target's
 * own argv (for 'run') and never handed to boost::program_options itself.
 *
 * @param   args    Command line arguments, excluding `argv[0]`.
 *
 * @return  Parsed options, or `std::nullopt` on a parsing error (already
 *          logged via `nova::log::error`).
 */
[[nodiscard]] auto parse_args(const std::vector<std::string>& all_args) -> std::optional<options> {
    std::vector<std::string> args;
    std::vector<std::string> forwarded_args;
    if (auto it = std::ranges::find(all_args, std::string("--")); it != all_args.end()) {
        args.assign(all_args.begin(), it);
        forwarded_args.assign(std::next(it), all_args.end());
    } else {
        args = all_args;
    }

    po::options_description desc("Baldr options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Print version and exit")
        ("project,p", po::value<std::string>()->default_value("."), "Project directory (default: current directory)")
        ("build-type,b", po::value<std::string>()->default_value("Debug"), "CMake build type/output subdir (default: 'Debug')")
        ("clean", po::bool_switch()->default_value(false), "For 'build': wipe the build directory before building (clean build)")
        ("cmake-define,D", po::value<std::vector<std::string>>()->composing(), "CMake define KEY=VALUE, repeatable")
        ("target,t", po::value<std::string>(), "Executable name to run (required for 'run')")
        ("build", po::bool_switch()->default_value(false), "For 'run': build the project first")
        ("debug", po::bool_switch()->default_value(false), "For 'run': launch the target under the configured debugger")
        ("image,i", po::value<std::string>(), "Docker image to use (required for 'docker')")
        ("command", po::value<std::string>(), "Command to run: 'build', 'run', 'docker'")
        ("args", po::value<std::vector<std::string>>()->multitoken(), "For 'docker': the container command")
    ;

    po::positional_options_description positional;
    positional.add("command", 1);
    positional.add("args", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(args).options(desc).positional(positional).run(), vm);

    if (vm.contains("help")) {
        print_help(std::cout);
        return std::nullopt;
    }

    po::notify(vm);

    if (vm.contains("version")) {
        std::cout << baldr_version() << '\n';
        return std::nullopt;
    }

    options result;
    result.project_dir = vm["project"].as<std::string>();
    result.build_type = vm["build-type"].as<std::string>();
    result.build_type_explicit = not vm["build-type"].defaulted();
    result.clean_build = vm["clean"].as<bool>();
    result.build_before_run = vm["build"].as<bool>();
    result.debug = vm["debug"].as<bool>();
    result.forwarded_args = std::move(forwarded_args);

    if (vm.contains("cmake-define")) {
        for (const auto& define: vm["cmake-define"].as<std::vector<std::string>>()) {
            auto eq_pos = define.find('=');
            if (eq_pos == std::string::npos) {
                throw nova::exception("Invalid -D/--define value '{}' (expected KEY=VALUE)", define);
            }
            result.cmake_defines[define.substr(0, eq_pos)] = define.substr(eq_pos + 1);
        }
    }

    if (vm.contains("target")) {
        result.target = vm["target"].as<std::string>();
    }

    if (vm.contains("image")) {
        result.image = vm["image"].as<std::string>();
    }

    if (vm.contains("args")) {
        result.docker_args = vm["args"].as<std::vector<std::string>>();
    }

    if (not vm.contains("command")) {
        print_help(std::cerr);
        throw nova::exception("No command given");
    }

    if (auto cmd = vm["command"].as<std::string>(); cmd == "build") {
        result.command = command_type::build;
    } else if (cmd == "run") {
        result.command = command_type::run;
        if (not result.target) {
            throw nova::exception("'run' requires -t/--target <name>");
        }
    } else if (cmd == "docker") {
        result.command = command_type::docker;
        if (not result.image) {
            throw nova::exception("'docker' requires -i/--image <name>");
        }
        if (result.docker_args.empty()) {
            throw nova::exception("'docker' requires a command to run inside the container");
        }
    } else {
        throw nova::exception("Unknown command: {}", cmd);
    }

    if (result.debug and result.command != command_type::run) {
        throw nova::exception("'--debug' only applies to 'run'");
    }

    return result;
}

} // namespace

/**
 * @brief   Baldr CLI entry point, invoked via `NOVA_MAIN`.
 *
 * @param   args    Command line arguments (`argv[0]` included), as a range
 *                  of `std::string_view`.
 */
auto entrypoint(auto args) -> int {
    nxs::rlog::init("baldr");

    std::vector<std::string> args_vec;
    for (const auto& arg: args | std::views::drop(1)) {
        args_vec.emplace_back(arg);
    }

    if (args_vec.empty()) {
        print_help(std::cerr);
        return EXIT_FAILURE;
    }

    const auto options = parse_args(args_vec);
    if (not options) {
        return EXIT_SUCCESS;
    }

    try {
        switch (options->command) {
            case command_type::build:
            case command_type::run: {
                auto cfg = baldr::load(options->project_dir);
                if (not cfg) {
                    throw nova::exception("Failed to load .baldr.yaml: {}", cfg.error().message);
                }

                auto build_type = options->build_type_explicit ? options->build_type : cfg->build_type;

                auto cmake_defines = cfg->cmake_defines;
                for (const auto& [key, value]: options->cmake_defines) {
                    cmake_defines[key] = value;
                }

                auto builder = baldr::builder{
                    options->project_dir,
                    build_type,
                    cmake_defines,
                    cfg->env,
                    cfg->debugger,
                    cfg->debugger_args
                };

                if (options->command == command_type::build) {
                    builder.build(options->clean_build);
                } else {
                    if (options->build_before_run) {
                        builder.build(options->clean_build);
                    }
                    builder.run(*options->target, options->forwarded_args, options->debug);
                }
                break;
            }
            case command_type::docker: {
                return baldr::docker_run(*options->image, options->docker_args);
            }
        }
    } catch (const nova::exception& ex) {
        nxs::rlog::failure(ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
