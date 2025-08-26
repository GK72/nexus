#include <baldr/command.hpp>
#include <baldr/progress.hpp>

#include <boost/program_options.hpp>

#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/main.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#define MAIN_ARG_PARSE(func, parse)                                             \
    int main(int argc, char* argv[]) {                                          \
        try {                                                                   \
            const auto args = parse(argc, argv);                                \
            if (not args.has_value()) {                                         \
                return EXIT_SUCCESS;                                            \
            }                                                                   \
            return func(*args);                                                 \
        } catch (nova::exception& ex) {                                         \
            nova::log::error(                                                   \
                "Exception caught in main: {}\n{}\n",                           \
                ex.what(),                                                      \
                ex.where(),                                                     \
                ex.backtrace()                                                  \
            );                                                                  \
        } catch (std::exception& ex) {                                          \
            nova::log::error("Exception caught in main: {}", ex.what());        \
        } catch (const char* msg) {                                             \
            nova::log::error("Exception caught in main: {}", msg);              \
        } catch (...) {                                                         \
            nova::log::error("Unknown exception caught in main");               \
        }                                                                       \
        return EXIT_FAILURE;                                                    \
    }

using namespace std::literals;
namespace po = boost::program_options;

constexpr auto DockerSock = "/var/run/docker.sock";

auto parse_args_run(const std::vector<std::string>& subargs)
        -> std::optional<boost::program_options::variables_map>
{
    auto arg_parser = po::options_description("Baldr Docker");      // TODO: Description.

    arg_parser.add_options()
        ("cmd", po::value<std::vector<std::string>>(), "TBD")
        ("help,h", "Show this help message")
    ;

    po::positional_options_description pos;
    pos.add("cmd", -1);

    po::variables_map args;
    auto parsed = po::command_line_parser(subargs)
        .options(arg_parser)
        .positional(pos)
        .allow_unregistered()
        .run();

    po::store(parsed, args);

    if (args.contains("help")) {
        std::cerr << arg_parser << "\n";
        return std::nullopt;
    }

    args.notify();
    args.insert({ "command"s, po::variable_value("run"s, false) });

    return args;
}

auto parse_args_test(const std::vector<std::string>& subargs)
        -> std::optional<boost::program_options::variables_map>
{
    auto arg_parser = po::options_description("Baldr Docker");      // TODO: Description.

    arg_parser.add_options()
        ("lines,l", po::value<int>()->default_value(1), "Number of lines to display")
        ("help,h", "Show this help message")
    ;

    po::positional_options_description pos;
    pos.add("cmd", -1);

    po::variables_map args;
    po::store(po::command_line_parser(subargs).options(arg_parser).run(), args);

    if (args.contains("help")) {
        std::cerr << arg_parser << "\n";
        return std::nullopt;
    }

    args.notify();
    args.insert({ "command"s, po::variable_value("test"s, false) });

    return args;
}

auto parse_args_docker(const std::vector<std::string>& subargs)
        -> std::optional<boost::program_options::variables_map>
{
    auto arg_parser = po::options_description("Baldr Docker");      // TODO: Description.

    arg_parser.add_options()
        ("image,i", po::value<std::string>()->required(), "Name of the Docker image")
        ("cmd", po::value<std::vector<std::string>>(), "Commands given to the container")
        ("socket", po::value<std::string>()->default_value(DockerSock), "Docker socker")
        ("help,h", "Show this help message")
    ;

    po::positional_options_description pos;
    pos.add("cmd", -1);

    po::variables_map args;
    auto parsed = po::command_line_parser(subargs)
        .options(arg_parser)
        .positional(pos)
        .allow_unregistered()
        .run();

    po::store(parsed, args);

    if (args.contains("help")) {
        std::cerr << arg_parser << "\n";
        return std::nullopt;
    }

    args.notify();
    args.insert({ "command"s, po::variable_value("docker"s, false) });

    return args;
}

auto parse_args(int argc, char* argv[]) -> std::optional<boost::program_options::variables_map> {
    auto arg_parser = po::options_description("Baldr");

    arg_parser.add_options()
        ("command", po::value<std::string>()->required(), "TBD")
        ("subargs", po::value<std::vector<std::string>>(), "Arguments for subcommand")
    ;

    auto positional_args = po::positional_options_description{ };
    positional_args
        .add("command", 1)
        .add("subargs", -1)
    ;

    po::variables_map args;

    auto parsed = po::command_line_parser(argc, argv)
        .options(arg_parser)
        .positional(positional_args)
        .allow_unregistered()
        .run();

    po::store(parsed, args);

    const auto command = args["command"].as<std::string>();
    const auto subargs = po::collect_unrecognized(parsed.options, po::include_positional);

    if (command == "help") {
        std::cerr << arg_parser << "\n";
        return std::nullopt;
    } else if (command == "test") {
        return parse_args_test(subargs);
    } else if (command == "docker") {
        return parse_args_docker(subargs);
    } else if (command == "run") {
        return parse_args_run(subargs);
    } else {
        throw nova::exception("Unsupported command {}, command");
    }
}

auto entrypoint([[maybe_unused]] const po::variables_map& args) -> int {
    nova::log::load_env_levels();
    nova::log::init("baldr");

    if (args["command"].as<std::string>() == "test") {
        auto progress = baldr::progress{ };
        progress.lines(args["lines"].as<int>());

        using namespace std::chrono_literals;
        for (int i = 1; i <= 10; ++i) {
            progress.msg("Line " + std::to_string(i));
            std::this_thread::sleep_for(200ms);
        }

        // progress.failure("Failed");
        progress.success("Finished");

        return EXIT_SUCCESS;
    }

    if (args["command"].as<std::string>() == "run") {
        // TODO: First one is "run", removed that during arg parsing.
        auto argv = args["cmd"].as<std::vector<std::string>>();
        argv.erase(std::begin(argv));

        auto cmd = baldr::command(argv);
        cmd.run();
        cmd.poll();
        // TODO(refact): Finish implementation.
        // while (cmd.poll()) {
        // }
        return cmd.wait();
    }

    return EXIT_SUCCESS;

    // const auto image = args["image"].as<std::string>();
    // const auto cmd = args["cmd"].as<std::vector<std::string>>();
    // const auto docker_socket = args["socket"].as<std::string>();

    // boost::asio::io_context io;
    // http_client client{io};
    // client.connect(docker_socket);

    // if (!docker_ping(client)) {
        // nova::topic_log::error("baldr", "Docker daemon not responding");
        // return 1;
    // }

    // // docker_pull(client, image);
    // const auto id = docker_create(client, image, cmd);
    // docker_start(client, id);
    // docker_logs(client, id);
    // const int code = docker_wait(client, id);

    // return code;
}

MAIN_ARG_PARSE(entrypoint, parse_args);
