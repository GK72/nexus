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

#include <libnxs/rlog.hpp>

#include <libnova/error.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>

#include <boost/program_options.hpp>

#include <fmt/format.h>

#include <cstdlib>
#include <iostream>
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
    out << "  -h, --help           Produce help message\n";
    out << "  -v, --version        Print version and exit\n";
    out << "  -p, --project <dir>  Project directory (default: current directory)\n";
    out << "\n";
    out << "  -t, --target <name>  Executable name to run (required for 'run')\n";
    out << "      --build          For 'run': build the project first\n";
    out << "\n";
    out << "Commands:\n";
    out << "  build      Configure (if needed) and build the project\n";
    out << "  run        Run the built target given via -t/--target\n";
}

enum class command_type {
    build,
    run,
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
    std::optional<std::string> target;
    bool build_before_run = false;
};

/**
 * @brief   Parse `args` (excluding the program name) into a `options`.
 *
 * @param   args    Command line arguments, excluding `argv[0]`.
 *
 * @return  Parsed options, or `std::nullopt` on a parsing error (already
 *          logged via `nova::log::error`).
 */
[[nodiscard]] auto parse_args(const std::vector<std::string>& args) -> std::optional<options> {
    po::options_description desc("Baldr options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("version,v", "Print version and exit")
        ("project,p", po::value<std::string>()->default_value("."), "Project directory (default: current directory)")
        ("target,t", po::value<std::string>(), "Executable name to run (required for 'run')")
        ("build", po::bool_switch()->default_value(false), "For 'run': build the project first")
        ("command", po::value<std::string>(), "Command to run: 'build', 'run'")
    ;

    po::positional_options_description positional;
    positional.add("command", 1);

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
    result.build_before_run = vm["build"].as<bool>();

    if (vm.contains("target")) {
        result.target = vm["target"].as<std::string>();
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
    } else {
        throw nova::exception("Unknown command: {}", cmd);
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
            case command_type::build: {
                auto builder = baldr::builder{ options->project_dir };
                builder.build();
                break;
            }
            case command_type::run: {
                auto builder = baldr::builder{ options->project_dir };
                if (options->build_before_run) {
                    builder.build();
                }
                builder.run(*options->target);
                break;
            }
        }
    } catch (const nova::exception& ex) {
        nxs::rlog::failure(ex.what());
        throw ex;
    }

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
