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
    out << "\n";
    out << "Commands:\n";
    out << "  build      Configure (if needed) and build the project\n";
}

enum class command_type {
    build,
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
        ("command", po::value<std::string>(), "Command to run: 'build'")
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

    if (not vm.contains("command")) {
        print_help(std::cerr);
        return std::nullopt;
    }

    if (auto cmd = vm["command"].as<std::string>(); cmd == "build") {
        result.command = command_type::build;
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

    switch (options->command) {
        case command_type::build:
            baldr::builder builder;
            builder.configure();
            builder.build();
            break;
    }

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
