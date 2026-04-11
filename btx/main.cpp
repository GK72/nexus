/**
 * Part of Nexus project.
 *
 * BTX CLI Tool.
 */

#include <libbtx/btx.hpp>
#include <libbtx/descriptor.hpp>
#include <libbtx/decoder.hpp>
#include <libbtx/generator.hpp>

#include <libnova/io.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>

#include <boost/program_options.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

namespace po = boost::program_options;

/**
 * @brief CLI options for the BTX tool.
 */
struct btx_options {
    std::string input_path;
    std::string output_path;
    std::string descriptor_path;
    bool        from_binary{false};
};

/**
 * @brief Parse command line arguments.
 * @param args Command line arguments.
 * @return Parsed options or std::nullopt if help was requested or an error occurred.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<btx_options> {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input,i", po::value<std::string>()->required(), "input file")
        ("output,o", po::value<std::string>(), "output file")
        ("descriptor,d", po::value<std::string>(), "descriptor file (YAML)")
        ("from-binary,f", po::bool_switch(), "convert from binary to BTX");

    po::positional_options_description p;
    p.add("input", 1);
    p.add("output", 1);

    po::variables_map vm;
    try {
        std::vector<std::string> args_vec;
        for (const auto& arg : args | std::views::drop(1)) {
            args_vec.emplace_back(arg);
        }

        po::store(po::command_line_parser(args_vec).options(desc).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout << "Usage: btx-tool [options] <input_file> [output_file]\n";
            std::cout << desc << "\n";
            return std::nullopt;
        }

        po::notify(vm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments: {}", e.what());
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
 * @brief   Read input data and convert to binary if necessary.
 *
 * @param   options CLI options.
 *
 * @return  Vector of binary data or error.
 */
[[nodiscard]] auto read_input(const btx_options& options) -> nova::expected<nova::bytes, nova::error> {
    const auto bin_res = nova::read_bin(options.input_path);
    if (not bin_res) {
        return nova::unexpected(bin_res.error());
    }

    if (options.from_binary) {
        return bin_res;
    }

    std::string       text(reinterpret_cast<const char*>(bin_res->data()), bin_res->size());
    std::stringstream in(text);
    std::stringstream out;
    if (const auto result = btx::to_binary(in, out); not result) {
        return nova::unexpected(result.error());
    }

    const std::string s = out.str();
    nova::bytes binary_data;
    binary_data.reserve(s.size());

    for (const char c : s) {
        binary_data.push_back(static_cast<std::byte>(c));
    }

    return binary_data;
}

/**
 * @brief   Handle output of binary data or BTX.
 *
 * @param   options     CLI options.
 * @param   binary_data Binary data to output.
 * @param   desc        Optional descriptor for annotated output.
 *
 * @return Vector of binary data or error.
 */
[[nodiscard]] auto write_output(
        const btx_options& options,
        const std::vector<std::byte>& binary_data,
        const std::optional<btx::descriptor>& desc = std::nullopt
) -> nova::expected<std::vector<std::byte>, nova::error> {
    if (options.from_binary) {
        std::ostringstream ss;
        if (desc) {
            if (const auto result = btx::format(*desc, nova::data_view(binary_data), ss); not result) {
                return nova::unexpected(result.error());
            }
        } else {
            if (const auto result = btx::from_binary(nova::data_view(binary_data), ss); not result) {
                return nova::unexpected(result.error());
            }
        }

        const std::string      s = ss.str();
        std::vector<std::byte> result;
        result.reserve(s.size());
        for (const char c : s) {
            result.push_back(static_cast<std::byte>(c));
        }
        return result;
    }

    return binary_data;
}

/**
 * @brief   Parse and display binary data using a descriptor.
 *
 * @param   options CLI options.
 *
 * @return  Vector of binary data or error.
 */
auto parse_with_descriptor(const btx_options& options) -> nova::expected<std::vector<std::byte>, nova::error> {
    const auto input_res = read_input(options);
    if (not input_res) {
        return nova::unexpected(input_res.error());
    }

    const auto& binary_data = *input_res;
    const auto  desc_res    = btx::load_descriptor_from_file(options.descriptor_path);
    if (not desc_res) {
        return nova::unexpected(desc_res.error());
    }

    const auto parse_res = btx::decode(*desc_res, nova::data_view(binary_data));
    if (not parse_res) {
        return nova::unexpected(parse_res.error());
    }

    nova::log::trace("Parsed message: {}", parse_res->name);
    for (const auto& field : parse_res->fields) {
        std::visit([&field](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::uint64_t>) {
                nova::log::trace("  {}: {} (0x{:x})", field.name, arg, arg);
            } else {
                nova::log::trace("  {}: {}", field.name, arg);
            }
        }, field.value);
    }

    return write_output(options, binary_data, *desc_res);
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

    auto result_data = [&]() -> nova::expected<std::vector<std::byte>, nova::error> {
        if (not options->descriptor_path.empty()) {
            return parse_with_descriptor(*options);
        }

        const auto input_res = read_input(*options);
        if (not input_res) {
            return nova::unexpected(input_res.error());
        }

        return write_output(*options, *input_res);
    }();

    if (not result_data) {
        nova::log::error("{}", result_data.error().message);
        return EXIT_FAILURE;
    }

    std::ofstream file_out;
    if (not options->output_path.empty()) {
        const auto mode = options->from_binary
            ? std::ios::out
            : (std::ios::out | std::ios::binary);

        file_out.open(options->output_path, mode);
        if (not file_out) {
            nova::log::error("Could not open output file: {}", options->output_path);
            return EXIT_FAILURE;
        }
    }

    std::ostream& out = file_out.is_open()
        ? file_out
        : std::cout;

    out.write(reinterpret_cast<const char*>(result_data->data()), static_cast<std::streamsize>(result_data->size()));

    if (not options->output_path.empty()) {
        if (options->from_binary) {
            nova::log::info("BTX written to: {}", options->output_path);
        } else {
            nova::log::info("Binary data written to: {}", options->output_path);
        }
    }

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
