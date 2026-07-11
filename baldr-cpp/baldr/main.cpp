#include <baldr/command.hpp>
#include <baldr/config.hpp>
#include <baldr/builder.hpp>
#include <baldr/line_reader.hpp>
#include <baldr/utils.hpp>
#include <baldr/log.hpp>

#include <libnova/log.hpp>
#include <libnova/main.hpp>

#include <boost/program_options.hpp>
#include <fmt/ranges.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

namespace po = boost::program_options;
using namespace std::literals;

/**
 * @brief   Available Baldr commands.
 */
enum class command_type {
    configure,
    build,
    run,
    debug,
    clean,
    doc,
    test,
    docker
};

/**
 * @brief   CLI options for Baldr.
 */
struct baldr_options {
    command_type command { command_type::build };
    std::optional<std::filesystem::path> project_path {};
    std::optional<std::string> build_dir {};
    std::optional<std::string> target {};
    std::vector<std::string> args {};
    std::optional<baldr::log::mode> log_mode {};
    bool help { false };
};

/**
 * @brief   Print the top-level usage message.
 */
void show_help() {
    std::cout << "Usage: baldr [global-options] <command> [subcommand-options]\n\n";
    std::cout << "Global Options:\n";
    std::cout << "  -p, --project <path>          Path to the project root\n";
    std::cout << "  --log-mode <standard|progress>  Force the logging output style\n";
    std::cout << "  -h, --help                    Show help message\n\n";
    std::cout << "Commands:\n";
    std::cout << "  configure  Generate CMake project\n";
    std::cout << "  build      Build project targets\n";
    std::cout << "  run        Execute a command locally\n";
    std::cout << "  debug      Spawn debugger for an executable\n";
    std::cout << "  test       Progress tracking demo\n";
    std::cout << "  clean      Remove build artifacts\n";
    std::cout << "  doc        Generate documentation\n";
}

/**
 * @brief   Parse options for the configure command.
 *
 * @param   options Options to populate (command, build_dir).
 * @param   subargs Command line arguments for the subcommand.
 *
 * @return  `true` on success, `false` on parse error.
 */
[[nodiscard]] bool parse_configure_args(baldr_options& options, const std::vector<std::string>& subargs) {
    po::options_description desc("Configure options");
    desc.add_options()
        ("build-dir,b", po::value<std::string>(), "Override build directory (relative to <project>/build)")
    ;

    po::variables_map subvm;
    try {
        po::store(po::command_line_parser(subargs).options(desc).run(), subvm);
        po::notify(subvm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments for configure: {}", e.what());
        return false;
    }

    options.command = command_type::configure;
    if (subvm.contains("build-dir")) {
        options.build_dir = subvm["build-dir"].as<std::string>();
    }
    return true;
}

/**
 * @brief   Parse options for the build command.
 *
 * @param   options Options to populate (command, build_dir, target).
 * @param   subargs Command line arguments for the subcommand.
 *
 * @return  `true` on success, `false` on parse error.
 */
[[nodiscard]] bool parse_build_args(baldr_options& options, const std::vector<std::string>& subargs) {
    po::options_description desc("Build options");
    desc.add_options()
        ("build-dir,b", po::value<std::string>(), "Override build directory (relative to <project>/build)")
        ("target,t", po::value<std::string>(), "Target to build")
    ;

    po::variables_map subvm;
    try {
        po::store(po::command_line_parser(subargs).options(desc).run(), subvm);
        po::notify(subvm);
    } catch (const std::exception& e) {
        nova::log::error("Error parsing arguments for build: {}", e.what());
        return false;
    }

    options.command = command_type::build;
    if (subvm.contains("build-dir")) {
        options.build_dir = subvm["build-dir"].as<std::string>();
    }
    if (subvm.contains("target")) {
        options.target = subvm["target"].as<std::string>();
    }

    nova::log::debug(
        "parse_build_args: build_dir={}, target={}",
        options.build_dir ? *options.build_dir : "<none, derived from build type>",
        options.target ? *options.target : "<none, will use config default>"
    );
    return true;
}

/**
 * @brief   Parse command line arguments.
 *
 * @param   args    Command line arguments.
 *
 * @return  Parsed options or `std::nullopt` if an error occurred.
 */
[[nodiscard]] auto parse_args(auto args) -> std::optional<baldr_options> {
    std::vector<std::string> args_vec;
    for (const auto& arg : args | std::views::drop(1)) {
        args_vec.emplace_back(arg);
    }

    nova::log::debug("parse_args: received {} argument(s): [{}]", args_vec.size(), fmt::join(args_vec, ", "));

    baldr_options options;
    std::string command;
    std::vector<std::string> subargs;

    for (std::size_t i = 0; i < args_vec.size(); ++i) {
        const std::string& arg = args_vec[i];
        if (arg == "-p" || arg == "--project") {
            if (i + 1 < args_vec.size()) {
                options.project_path = args_vec[++i];
            }
        } else if (arg.rfind("--log-mode", 0) == 0) {
            std::string value;
            if (auto eq = arg.find('='); eq != std::string::npos) {
                value = arg.substr(eq + 1);
            } else if (i + 1 < args_vec.size()) {
                value = args_vec[++i];
            }
            if (value == "standard") {
                options.log_mode = baldr::log::mode::standard;
            } else if (value == "progress") {
                options.log_mode = baldr::log::mode::progress;
            } else {
                nova::log::error("Invalid value for --log-mode: '{}'", value);
                return std::nullopt;
            }
        } else if (arg == "-h" || arg == "--help") {
            show_help();
            return baldr_options { .help = true };
        } else if (command.empty() && !arg.empty() && arg[0] != '-') {
            command = arg;
        } else if (command.empty() && (arg == "--image" || arg == "-i")) {
            command = "docker";
            subargs.push_back(arg);
        } else {
            subargs.push_back(arg);
        }
    }

    if (command.empty()) {
        show_help();
        return baldr_options { .help = true };
    }

    options.args = subargs;

    nova::log::debug(
        "parse_args: command='{}', project_path='{}', subargs=[{}]",
        command,
        options.project_path ? options.project_path->string() : "<cwd>",
        fmt::join(subargs, ", ")
    );

    if (command == "configure") {
        return parse_configure_args(options, subargs) ? std::optional{ options } : std::nullopt;
    }
    if (command == "build") {
        return parse_build_args(options, subargs) ? std::optional{ options } : std::nullopt;
    }
    if (command == "run") {
        options.command = command_type::run;
        return options;
    }
    if (command == "debug") {
        options.command = command_type::debug;
        return options;
    }
    if (command == "clean") {
        options.command = command_type::clean;
        return options;
    }
    if (command == "doc") {
        options.command = command_type::doc;
        return options;
    }
    if (command == "test") {
        options.command = command_type::test;
        return options;
    }
    if (command == "docker") {
        options.command = command_type::docker;
        return options;
    }

    nova::log::error("Unknown command: {}", command);
    return std::nullopt;
}

/**
 * @brief   Stream a command's output through the unified logging API until it exits.
 *
 * @param   run_args    Command and its arguments.
 *
 * @return  Exit code of the command.
 */
[[nodiscard]] auto stream_command(const std::vector<std::string>& run_args) -> int {
    auto cmd = baldr::command(run_args);
    cmd.run();
    baldr::line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), !chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();
    return cmd.wait();
}

/**
 * @brief   Execute a command locally, resolving executables from the build tree.
 *
 * @param   options         Parsed options carrying the forwarded arguments.
 * @param   build_dir       Path to the build directory to search for executables.
 *
 * @return  Exit code of the command.
 */
[[nodiscard]] auto run_local(const baldr_options& options, const std::filesystem::path& build_dir) -> int {
    if (options.args.empty()) {
        std::cerr << "Usage: baldr run <command> [args...]\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> run_args = options.args;
    const std::string exe = run_args[0];

    if (!std::filesystem::exists(exe)) {
        auto found = baldr::utils::find_executables(build_dir);
        for (const auto& f : found) {
            if (f.filename() == exe) {
                run_args[0] = f.string();
                break;
            }
        }
    }

    return stream_command(run_args);
}

/**
 * @brief   Docker runner compatibility shim for the run script.
 *
 * @param   options     Parsed options carrying the forwarded arguments.
 *
 * @return  Exit code of the command.
 */
[[nodiscard]] auto run_docker(const baldr_options& options) -> int {
    const std::vector<std::string>& subargs = options.args;

    std::vector<std::string> run_args;
    bool found_sep = false;
    for (std::size_t k = 0; k < subargs.size(); ++k) {
        if (subargs[k] == "--") {
            found_sep = true;
            for (std::size_t j = k + 1; j < subargs.size(); ++j) {
                run_args.push_back(subargs[j]);
            }
            break;
        }
    }
    if (!found_sep && subargs.size() >= 2) {
        // skip -i/--image and the image name
        for (std::size_t k = 2; k < subargs.size(); ++k) {
            run_args.push_back(subargs[k]);
        }
    }

    if (run_args.empty()) {
        nova::log::info("Docker simulated");
        return EXIT_SUCCESS;
    }

    return stream_command(run_args);
}

/**
 * @brief   Run the logging mode demo.
 *
 * @return  `EXIT_SUCCESS`.
 */
[[nodiscard]] auto run_test() -> int {
    for (int i = 1; i <= 5; ++i) {
        nova::log::info("Testing logging... {}", i);
        std::this_thread::sleep_for(200ms);
    }
    nova::log::info("Test finished");
    return EXIT_SUCCESS;
}

/**
 * @brief   Compose the build directory, mirroring Baldr's original layout.
 *
 * The build directory is placed under `<project_root>/build`. Unless a custom
 * sub-directory is supplied, its name encodes the build type (lower-cased) and,
 * when configured, the C++ compiler, e.g. `debug` or `debug-clang++`.
 *
 * @param   project_root    Path to the project root.
 * @param   config          Loaded configuration (build type, compiler).
 * @param   custom_dir      Optional override for the build sub-directory name.
 *
 * @return  Absolute path to the build directory.
 */
[[nodiscard]] auto compose_build_dir(
    const std::filesystem::path& project_root,
    const baldr::Config& config,
    const std::optional<std::string>& custom_dir
) -> std::filesystem::path {
    std::string dir;
    if (custom_dir) {
        dir = *custom_dir;
    } else {
        dir = config.build_type;
        std::ranges::transform(dir, dir.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (config.cxx) {
            dir += "-" + std::filesystem::path(*config.cxx).filename().string();
        }
    }
    return project_root / "build" / dir;
}

/**
 * @brief   Main entry point for Baldr.
 *
 * @param   args    Command line arguments.
 *
 * @return  `EXIT_SUCCESS` or `EXIT_FAILURE`.
 */
auto entrypoint(auto args) -> int {
    nova::log::load_env_levels();
    nova::log::init("baldr");

    const auto options = parse_args(args);
    if (not options) {
        return EXIT_FAILURE;
    }

    if (options->help) {
        return EXIT_SUCCESS;
    }

    baldr::log::init(baldr::log::resolve_mode(options->log_mode));

    auto config = baldr::Config::load(options->project_path);
    auto builder = baldr::Builder{config};
    auto project_root = options->project_path ? *options->project_path : std::filesystem::current_path();
    auto build_dir = compose_build_dir(project_root, config, options->build_dir);

    nova::log::debug(
        "entrypoint: project_root='{}', build_type='{}', build_dir='{}'",
        project_root.string(),
        config.build_type,
        build_dir.string()
    );

    switch (options->command) {
        case command_type::configure:
            return builder.configure(project_root.string(), build_dir.string());

        case command_type::build: {
            std::string target = options->target ? *options->target : config.target;
            nova::log::debug(
                "entrypoint: build target='{}' (from {}), build_dir='{}'",
                target,
                options->target ? "-t/--target" : "config default",
                build_dir.string()
            );
            return builder.build(build_dir.string(), target);
        }

        case command_type::run:
            return run_local(*options, build_dir);

        case command_type::debug:
            return builder.debug(build_dir.string(), options->args);

        case command_type::clean: {
            std::filesystem::remove_all(build_dir);
            nova::log::info("Cleaned build directory: {}", build_dir.string());
            return EXIT_SUCCESS;
        }

        case command_type::doc:
            return stream_command({"./tools/doc.sh"});

        case command_type::test:
            return run_test();

        case command_type::docker:
            return run_docker(*options);
    }

    return EXIT_FAILURE;
}

NOVA_MAIN(entrypoint);
