/**
 * Part of @part_of@
 *
 * @brief   The command line: `@name@ [name]`, `@name@ --version`.
 *
 * @author      @author@
 * @created     @date@
 * @updated     @date@
 * @status      experimental
 * @reviewed    none
 */

#include <lib@name@/greeting.hpp>

#include <@name@/gen/build_info.hpp>

#include <libnova/exception.hpp>               // NOVA_MAIN catches nova::exception; Nova 0.15.0's main.hpp lacks it
#include <libnova/main.hpp>

#include <boost/program_options.hpp>

#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace po = boost::program_options;

namespace {

/**
 * @brief   What the command line asks for.
 */
struct options_t {
    std::string name;
};

/**
 * @brief   The *visible* options, those `print_help()` lists. The positional `name` is kept in a
 *          separate, hidden description, so it doesn't show up as a fake `--name` flag.
 */
[[nodiscard]] auto build_options_description() -> po::options_description {
    po::options_description desc("Options");
    desc.add_options()
        ("help,h",  "Print this help")
        ("version", "Print the version, the commit and the branch")
    ;
    return desc;
}

void print_help(std::ostream& out, const po::options_description& desc) {
    out << "Usage: @name@ [options] [name]\n";
    out << "\n";
    out << "Greets someone, or the world.\n";
    out << "\n";
    out << desc;
}

/**
 * @brief   Parse `args` into an `options_t`.
 *
 * @param   args    Command line arguments, excluding `argv[0]`.
 *
 * @return  The options, or `std::nullopt` if `--help`/`--version` was given (already printed
 *          to stdout).
 *
 * @throws  `po::error` on an unknown option or a wrong number of arguments.
 */
[[nodiscard]] auto parse_args(const std::vector<std::string>& args) -> std::optional<options_t> {
    const auto desc = build_options_description();

    po::options_description positional_desc("Positional arguments");
    positional_desc.add_options()
        ("name", po::value<std::string>()->default_value(""), "Who to greet")
    ;

    po::options_description all_options;
    all_options.add(desc).add(positional_desc);

    po::positional_options_description positional;
    positional.add("name", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(args).options(all_options).positional(positional).run(), vm);

    if (vm.contains("help")) {
        print_help(std::cout, desc);
        return std::nullopt;
    }

    po::notify(vm);

    if (vm.contains("version")) {
        std::println("@name@ {}", @namespace@::build::full_version());
        return std::nullopt;
    }

    options_t result;
    result.name = vm["name"].as<std::string>();
    return result;
}

/**
 * @brief   A range of command-line arguments, as produced by `NOVA_MAIN` (`argv`, transformed to
 *          `std::string_view`).
 */
template <typename T>
concept arg_range =
    std::ranges::input_range<T> and
    std::convertible_to<std::ranges::range_reference_t<T>, std::string_view>;

} // namespace

/**
 * @brief   The entry point, invoked via `NOVA_MAIN`, which logs what escapes as an exception.
 *
 * @param   args    Command line arguments, `argv[0]` included.
 */
auto entrypoint(arg_range auto args) -> int {
    std::vector<std::string> args_vec;
    for (const auto& arg : args | std::views::drop(1)) {
        args_vec.emplace_back(arg);
    }

    std::optional<options_t> options;
    try {
        options = parse_args(args_vec);
    } catch (const po::error& ex) {
        std::println(stderr, "{}", ex.what());
        print_help(std::cerr, build_options_description());
        return EXIT_FAILURE;
    }

    if (not options) {
        return EXIT_SUCCESS;
    }

    std::println("{}", @namespace@::greet(options->name));
    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
