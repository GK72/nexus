/**
 * Part of Nexus project.
 *
 * Random Message Generator CLI Tool.
 */

#include <libbtx/descriptor.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <ranges>
#include <optional>

namespace po = boost::program_options;

/**
 * @struct gen_options
 * @brief Command line options for msg-gen.
 */
struct gen_options {
    std::string descriptor_path;
    std::string output_path;
};

/**
 * @brief Parse command line arguments for msg-gen.
 * @param args Command line arguments.
 * @return Parsed options or nullopt if error/help.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<gen_options> {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("descriptor,d", po::value<std::string>(), "descriptor file (YAML)")
        ("output,o", po::value<std::string>(), "output file (binary)");

    po::positional_options_description p;
    p.add("descriptor", 1);
    p.add("output", 1);

    po::variables_map vm;
    try {
        std::vector<std::string> args_vec;
        for (const auto& arg : args | std::views::drop(1)) {
            args_vec.emplace_back(arg);
        }

        po::store(po::command_line_parser(args_vec).options(desc).positional(p).run(), vm);
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments: {}", e.what());
        return std::nullopt;
    }

    if (vm.count("help")) {
        std::cout << "Usage: msg-gen [options] <descriptor_file> [output_file]\n";
        std::cout << desc << "\n";
        return std::nullopt;
    }

    if (not vm.count("descriptor")) {
        nova::log::error("Missing descriptor file.");
        std::cout << "Usage: msg-gen [options] <descriptor_file> [output_file]\n";
        return std::nullopt;
    }

    return gen_options{
        .descriptor_path = vm["descriptor"].as<std::string>(),
        .output_path = vm.count("output") ? vm["output"].as<std::string>() : ""
    };
}

/**
 * @brief Main entrypoint for msg-gen.
 * @param args Command line arguments.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
auto entrypoint(auto args) -> int {
    nova::log::init("msg-gen");

    const auto options = parse_args(args);
    if (not options) {
        return EXIT_FAILURE;
    }

    auto desc_res = btx::descriptor::load(options->descriptor_path);
    if (not desc_res) {
        nova::log::error("Failed to load descriptor: {}", desc_res.error().message);
        return EXIT_FAILURE;
    }

    auto gen_res = desc_res->generate_random();
    if (not gen_res) {
        nova::log::error("Generation error: {}", gen_res.error().message);
        return EXIT_FAILURE;
    }

    if (not options->output_path.empty()) {
        std::ofstream out(options->output_path, std::ios::binary);
        if (not out) {
            nova::log::error("Could not open output file: {}", options->output_path);
            return EXIT_FAILURE;
        }
        for (auto b : *gen_res) {
            out.put(static_cast<char>(b));
        }
        nova::log::trace("Random binary data written to: {}", options->output_path);
    } else {
        for (auto b : *gen_res) {
            std::cout.put(static_cast<char>(b));
        }
    }

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
