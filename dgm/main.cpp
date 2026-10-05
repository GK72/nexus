/**
 * Part of Nexus DGM (Diagram as Code).
 *
 * DGM CLI Tool: renders a `.dgm` file to SVG.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @date        2026-10-05
 */

#include <dgm/diagram.hpp>

#include <libnova/io.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>

#include <boost/program_options.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace po = boost::program_options;

/**
 * @brief   CLI options for the DGM tool.
 */
struct dgm_options {
    std::string input_path {};
    std::string output_path {};
    bool help { false };
};

/**
 * @brief   Parse command line arguments.
 *
 * @param   args    Command line arguments, including the program name.
 *
 * @return  Parsed options or `std::nullopt` on error.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<dgm_options> {
    std::vector<std::string> args_vec;
    for (const auto& arg : args | std::views::drop(1)) {
        args_vec.emplace_back(arg);
    }

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Produce help message")
        ("input,i", po::value<std::string>()->required(), "Input .dgm file")
        ("output,o", po::value<std::string>(), "Output .svg file (default: stdout)")
    ;
    po::positional_options_description p;
    p.add("input", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(args_vec).options(desc).positional(p).run(), vm);
        if (vm.contains("help")) {
            std::cout << "Usage: dgm-tool [options] <input.dgm>\n";
            std::cout << desc << "\n";
            return dgm_options { .help = true };
        }
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments: {}", e.what());
        return std::nullopt;
    }

    return dgm_options {
        .input_path  = vm["input"].as<std::string>(),
        .output_path = vm.contains("output") ? vm["output"].as<std::string>() : "",
        .help        = false
    };
}

/**
 * @brief   Main entry point for the DGM tool.
 */
auto entrypoint(auto args) -> int {
    nova::log::init("dgm");

    const auto options = parse_args(args);
    if (not options) {
        return EXIT_FAILURE;
    }
    if (options->help) {
        return EXIT_SUCCESS;
    }

    const auto result = nova::read_file(options->input_path)
        .and_then([](const std::string& source) { return dgm::parse(source); })
        .and_then([](const dgm::spec& spec) { return dgm::layout(spec); });

    if (not result) {
        nova::log::error("{}: {}", options->input_path, result.error().message);
        return EXIT_FAILURE;
    }

    const auto svg = dgm::to_svg(*result);

    if (options->output_path.empty()) {
        std::cout << svg;
        return EXIT_SUCCESS;
    }

    std::ofstream out(options->output_path);
    if (not out) {
        nova::log::error("Could not open output file: {}", options->output_path);
        return EXIT_FAILURE;
    }
    out << svg;
    nova::log::info("SVG written to: {}", options->output_path);

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
