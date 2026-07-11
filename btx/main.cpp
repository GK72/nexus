/**
 * Part of BTX Toolset.
 *
 * BTX CLI Tool.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-11
 */

#include <libbtx/btx.hpp>
#include <liblexy/descriptor.hpp>
#include <liblexy/decoder.hpp>
#include <liblexy/generator.hpp>

#include <libnova/io.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>

#include <boost/program_options.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace po = boost::program_options;

/**
 * @brief   Substitute for `nova::expected<void, ...>`.
 */
struct empty {};

/**
 * @brief   Type of command for the BTX tool.
 */
enum class command_type {
    encode,
    decode
};

/**
 * @brief   CLI options for the BTX tool.
 */
struct btx_options {
    command_type command { command_type::encode };
    std::string input_path {};
    std::string output_path {};
    std::string descriptor_path {};
    bool help { false };
};

/**
 * @brief   Add common options to the description.
 *
 * @param   desc    Options description to add to.
 * @param   p       Positional options to add to.
 */
void add_common_options(po::options_description& desc, po::positional_options_description& p) {
    desc.add_options()
        ("help,h", "Produce help message")
        ("input,i", po::value<std::string>()->required(), "Input file")
        ("output,o", po::value<std::string>(), "Output file")
    ;
    p.add("input", 1);
    p.add("output", 1);
}

/**
 * @brief   Parse options for the encode command.
 *
 * @param   args    Command line arguments (excluding the subcommand itself).
 *
 * @return  Parsed options or `std::nullopt` on error or help.
 */
[[nodiscard]] auto parse_encode_args(const std::vector<std::string>& args) -> std::optional<btx_options> {
    po::options_description desc("Options for encode");
    po::positional_options_description p;
    add_common_options(desc, p);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(args).options(desc).positional(p).run(), vm);
        if (vm.contains("help")) {
            std::cout << "Usage: btx-tool encode [options] <input_file> [output_file]\n";
            std::cout << desc << "\n";
            return btx_options { .help = true };
        }
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments for encode: {}", e.what());
        return std::nullopt;
    }

    return btx_options {
        .command     = command_type::encode,
        .input_path  = vm["input"].as<std::string>(),
        .output_path = vm.contains("output") ? vm["output"].as<std::string>() : ""
    };
}

/**
 * @brief   Parse options for the decode command.
 *
 * @param   args    Command line arguments (excluding the subcommand itself).
 *
 * @return  Parsed options or `std::nullopt` on error or help.
 */
[[nodiscard]] auto parse_decode_args(const std::vector<std::string>& args) -> std::optional<btx_options> {
    po::options_description desc("Options for decode");
    po::positional_options_description p;

    add_common_options(desc, p);

    desc.add_options()
        ("descriptor,d", po::value<std::string>(), "Descriptor file (YAML)")
    ;

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(args).options(desc).positional(p).run(), vm);
        if (vm.contains("help")) {
            std::cout << "Usage: btx-tool decode [options] <input_file> [output_file]\n";
            std::cout << desc << "\n";
            return btx_options { .help = true };
        }
        po::notify(vm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments for decode: {}", e.what());
        return std::nullopt;
    }

    return btx_options {
        .command         = command_type::decode,
        .input_path      = vm["input"].as<std::string>(),
        .output_path     = vm.contains("output") ? vm["output"].as<std::string>() : "",
        .descriptor_path = vm.contains("descriptor") ? vm["descriptor"].as<std::string>() : ""
    };
}

/**
 * @brief   Parse command line arguments.
 *
 * @param   args    Command line arguments.
 *
 * @return  Parsed options or `std::nullopt` if help was requested or an error occurred.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<btx_options> {
    std::vector<std::string> args_vec;
    for (const auto& arg : args | std::views::drop(1)) {
        args_vec.emplace_back(arg);
    }

    if (args_vec.empty()) {
        std::cout << "Usage: btx-tool <command> [options]\n";
        std::cout << "Commands:\n";
        std::cout << "  encode    Convert from BTX to binary\n";
        std::cout << "  decode    Convert from binary to BTX\n";
        return std::nullopt;
    }

    const std::string command = args_vec[0];
    if (command == "help" or command == "-h" or command == "--help") {
        std::cout << "Usage: btx-tool <command> [options]\n";
        std::cout << "Commands:\n";
        std::cout << "  encode    Convert from BTX to binary\n";
        std::cout << "  decode    Convert from binary to BTX\n";
        return btx_options { .help = true };
    }

    std::vector<std::string> sub_args(args_vec.begin() + 1, args_vec.end());

    if (command == "encode") {
        return parse_encode_args(sub_args);
    }

    if (command == "decode") {
        return parse_decode_args(sub_args);
    }

    nova::log::error("Unknown command: {}", command);
    return std::nullopt;
}

/**
 * @brief   Write output data to file or stdout.
 *
 * @param   options CLI options.
 * @param   data    Data to output.
 *
 * @return  Data that was written or error.
 */
[[nodiscard]] auto write_output(nova::data_view data, const btx_options& options)
        -> nova::expected<empty, nova::error>
{
    std::ofstream file_out;
    if (not options.output_path.empty()) {
        const auto mode = options.command == command_type::decode
            ? std::ios::out
            : (std::ios::out | std::ios::binary);

        file_out.open(options.output_path, mode);
        if (not file_out) {
            return nova::unexpected(nova::error("Could not open output file: " + options.output_path));
        }
    }

    std::ostream& out = file_out.is_open()
        ? file_out
        : std::cout;

    out.write(data.char_ptr(), static_cast<std::streamsize>(data.size()));

    if (not options.output_path.empty()) {
        if (options.command == command_type::decode) {
            nova::log::info("BTX written to: {}", options.output_path);
        } else {
            nova::log::info("Binary data written to: {}", options.output_path);
        }
    }

    return empty{ };
}

/**
 * @brief   Trace logging for the decoded fields.
 */
auto trace_fields(const lexy::message_data& msg) {
    nova::log::trace("Parsed message: {}", msg.name);
    for (const auto& field : msg.fields) {
        std::visit(
            [&field](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::uint64_t>) {
                    nova::log::trace("  {}: {} (0x{:x})", field.name, arg, arg);
                } else {
                    nova::log::trace("  {}: {}", field.name, arg);
                }
            }, field.value
        );
    }
}

/**
 * @brief   Decoding binary data using a descriptor and annotate it.
 *
 * @param   options CLI options.
 * @param   data    Binary data to decode.
 *
 * @return  Annotated BTX data or error.
 */
auto annotated_decode(const btx_options& options, const nova::bytes& data)
        -> nova::expected<nova::bytes, nova::error>
{
    const auto descriptor = lexy::load_descriptor(std::filesystem::path{ options.descriptor_path });
    if (not descriptor) {
        return nova::unexpected(descriptor.error());
    }

    const auto decoded = lexy::decode(*descriptor, nova::data_view(data));
    if (not decoded) {
        return nova::unexpected(decoded.error());
    }

    trace_fields(*decoded);

    return lexy::format(*descriptor, nova::data_view(data));
}

/**
 * @brief   Main entry point for the BTX tool.
 */
auto entrypoint(auto args) -> int {
    nova::log::init("btx");

    const auto options = parse_args(args);
    if (not options) {
        return EXIT_FAILURE;
    }

    if (options->help) {
        return EXIT_SUCCESS;
    }

    const auto result = nova::read_bin(options->input_path)
        .and_then([&options](const nova::bytes& data) -> nova::expected<nova::bytes, nova::error> {
            if (options->command == command_type::encode) {
                return btx::to_binary(nova::data_view(data).as_string());
            }

            if (not options->descriptor_path.empty()) {
                return annotated_decode(*options, data);
            }

            return btx::from_binary(nova::data_view(data));
        })
        .and_then([&options](const nova::bytes& data) {
            return write_output(nova::data_view(data), *options);
        });

    if (not result) {
        nova::log::error("{}", result.error().message);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
