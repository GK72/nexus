/**
 * Part of Nexus project.
 *
 * BTX CLI Tool.
 */

#include <libbtx/btx.hpp>
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
#include <cstddef>
#include <cstdlib>

#include <sstream>

namespace po = boost::program_options;

/**
 * @brief   CLI options for the BTX tool.
 */
struct btx_options {
    std::string input_path;
    std::string output_path;
    std::string descriptor_path;
    bool        from_binary{false};
};

/**
 * @brief   Parse command line arguments.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<btx_options> {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input,i", po::value<std::string>(), "input file")
        ("output,o", po::value<std::string>(), "output file")
        ("descriptor,d", po::value<std::string>(), "descriptor file (YAML)")
        ("from-binary,f", po::bool_switch(), "convert from binary to BTX");

    po::positional_options_description p;
    p.add("input", 1);
    p.add("output", 1);

    po::variables_map vm;
    try {
        // Convert range of string_view to vector of strings for Boost.PO
        // Skipping the first element (executable name)
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
        std::cout << "Usage: btx-tool [options] <input_file> [output_file]\n";
        std::cout << desc << "\n";
        return std::nullopt;
    }

    if (not vm.count("input")) {
        nova::log::error("Missing input file.");
        std::cout << "Usage: btx-tool [options] <input_file> [output_file]\n";
        return std::nullopt;
    }

    return btx_options{
        .input_path      = vm["input"].as<std::string>(),
        .output_path     = vm.count("output") ? vm["output"].as<std::string>() : "",
        .descriptor_path = vm.count("descriptor") ? vm["descriptor"].as<std::string>() : "",
        .from_binary     = vm["from-binary"].as<bool>()
    };
}

/**
 * @brief   Perform the conversion from BTX to binary.
 */
auto convert_to_binary(const std::string& input_path, std::ostream& out) -> int {
    std::ifstream in(input_path);
    if (not in) {
        nova::log::error("Could not open input file: {}", input_path);
        return EXIT_FAILURE;
    }

    if (const auto result = btx::to_binary(in, out); not result) {
        nova::log::error("Conversion error: {}", result.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief   Perform the conversion from binary to BTX.
 */
auto convert_from_binary(const std::string& input_path, std::ostream& out) -> int {
    std::ifstream in(input_path, std::ios::binary);
    if (not in) {
        nova::log::error("Could not open input file: {}", input_path);
        return EXIT_FAILURE;
    }

    // Read all binary data
    std::vector<std::byte> binary_data;
    char ch = '\0';
    while (in.get(ch)) {
        binary_data.push_back(static_cast<std::byte>(ch));
    }

    nova::data_view view(binary_data);
    if (const auto result = btx::from_binary(view, out); not result) {
        nova::log::error("Conversion error: {}", result.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief   Read input data based on options.
 */
auto read_input(const btx_options& options) -> nova::expected<std::vector<std::byte>, nova::error> {
    std::vector<std::byte> binary_data;
    if (options.from_binary) {
        std::ifstream in(options.input_path, std::ios::binary);
        if (not in) {
            return nova::unexpected(nova::error("Could not open input file: " + options.input_path));
        }
        char ch = '\0';
        while (in.get(ch)) {
            binary_data.push_back(static_cast<std::byte>(ch));
        }
    } else {
        std::ifstream in(options.input_path);
        if (not in) {
            return nova::unexpected(nova::error("Could not open input file: " + options.input_path));
        }
        std::stringstream ss;
        if (const auto result = btx::to_binary(in, ss); not result) {
            return nova::unexpected(result.error());
        }
        const std::string s = ss.str();
        for (const char c : s) {
            binary_data.push_back(static_cast<std::byte>(c));
        }
    }
    return binary_data;
}

/**
 * @brief   Handle output based on options.
 */
auto handle_output(const btx_options& options, 
                  const btx::descriptor& desc, 
                  const std::vector<std::byte>& binary_data) -> int {
    if (options.output_path.empty()) {
        if (options.from_binary) {
            if (const auto result = desc.to_btx(nova::data_view(binary_data), std::cout); not result) {
                nova::log::error("Annotation error: {}", result.error().message);
                return EXIT_FAILURE;
            }
        } else {
            for (const auto b : binary_data) {
                std::cout.put(static_cast<char>(b));
            }
        }
        return EXIT_SUCCESS;
    }

    if (options.from_binary) {
        std::ofstream out(options.output_path);
        if (not out) {
            nova::log::error("Could not open output file: {}", options.output_path);
            return EXIT_FAILURE;
        }
        if (const auto result = desc.to_btx(nova::data_view(binary_data), out); not result) {
            nova::log::error("Annotation error: {}", result.error().message);
            return EXIT_FAILURE;
        }
        nova::log::trace("Annotated BTX written to: {}", options.output_path);
    } else {
        std::ofstream out(options.output_path, std::ios::binary);
        if (not out) {
            nova::log::error("Could not open output file: {}", options.output_path);
            return EXIT_FAILURE;
        }
        for (const auto b : binary_data) {
            out.put(static_cast<char>(b));
        }
        nova::log::trace("Binary data written to: {}", options.output_path);
    }
    return EXIT_SUCCESS;
}

/**
 * @brief   Parse and display binary data using a descriptor.
 */
auto parse_with_descriptor(const btx_options& options) -> int {
    const auto input_res = read_input(options);
    if (not input_res) {
        nova::log::error("{}", input_res.error().message);
        return EXIT_FAILURE;
    }
    const auto& binary_data = *input_res;

    const auto desc_res = btx::descriptor::load(options.descriptor_path);
    if (not desc_res) {
        nova::log::error("Failed to load descriptor: {}", desc_res.error().message);
        return EXIT_FAILURE;
    }

    const auto parse_res = desc_res->parse(nova::data_view(binary_data));
    if (not parse_res) {
        nova::log::error("Parsing error: {}", parse_res.error().message);
        return EXIT_FAILURE;
    }

    nova::log::trace("Parsed message: {}", parse_res->name);
    for (const auto& field : parse_res->fields) {
        std::visit([&field](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, uint64_t>) {
                nova::log::trace("  {}: {} (0x{:x})", field.name, arg, arg);
            } else {
                nova::log::trace("  {}: {}", field.name, arg);
            }
        }, field.value);
    }

    return handle_output(options, *desc_res, binary_data);
}

/**
 * @brief   Main entry point for the BTX tool.
 */
auto entrypoint(auto args) -> int {
    nova::log::init("btx-tool");

    const auto options = parse_args(args);
    if (not options) {
        return EXIT_FAILURE;
    }

    if (not options->descriptor_path.empty()) {
        return parse_with_descriptor(*options);
    }

    if (not options->output_path.empty()) {
        if (options->from_binary) {
            std::ofstream out(options->output_path);
            if (not out) {
                nova::log::error("Could not open output file: {}", options->output_path);
                return EXIT_FAILURE;
            }
            return convert_from_binary(options->input_path, out);
        } else {
            std::ofstream out(options->output_path, std::ios::binary);
            if (not out) {
                nova::log::error("Could not open output file: {}", options->output_path);
                return EXIT_FAILURE;
            }
            return convert_to_binary(options->input_path, out);
        }
    }

    if (options->from_binary) {
        return convert_from_binary(options->input_path, std::cout);
    }

    return convert_to_binary(options->input_path, std::cout);
}

NOVA_MAIN(entrypoint);
