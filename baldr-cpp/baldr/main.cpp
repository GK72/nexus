#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/main.hpp>

#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
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

using boost::asio::local::stream_protocol;
namespace po = boost::program_options;

constexpr auto DockerSock = "/var/run/docker.sock";
constexpr auto DefaultApiVersion = "v1.43";

// ------------------------------------------------------------
// Utility: Return API version prefix, configurable via env var.
// ------------------------------------------------------------
[[nodiscard]] std::string api_prefix() {
    if (const char* v = std::getenv("DOCKER_API_VERSION")) {
        std::string ver = v;
        if (!ver.empty() && ver[0] != 'v') ver = "v" + ver;
        return "/" + ver;
    }
    return std::string("/") + DefaultApiVersion;
}

class http_client {
public:
    explicit http_client(boost::asio::io_context& io)
        : m_socket{io} {}

    void connect(const std::string& path = DockerSock) {
        boost::asio::local::stream_protocol::endpoint ep{path};
        m_socket.connect(ep);
    }

    [[nodiscard]] auto request(
            const std::string& method,
            const std::string& target,
            const nlohmann::json& body = {},
            const std::vector<std::pair<std::string, std::string>>& headers = {})
        -> nova::bytes
    {
        namespace beast = boost::beast;
        namespace http = boost::beast::http;

        http::request<http::string_body> req;
        req.method(http::string_to_verb(method));
        req.target(target);
        req.version(11);
        req.set(http::field::host, "docker");
        if (!body.empty()) {
            req.set(http::field::content_type, "application/json");
            req.body() = body;
            req.prepare_payload();
        }
        for (const auto& [k, v] : headers) {
            req.set(k, v);
        }

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;

        http::write(m_socket, req);
        http::read(m_socket, buffer, res);

        return nova::data_view{ res.body() }.to_vec();
    }

    [[nodiscard]] nlohmann::json request_json(
        const std::string& method,
        const std::string& target,
        const nlohmann::json& body = {},
        const std::vector<std::pair<std::string, std::string>>& headers = {}) {

        const std::string body_str = body.empty() ? std::string{} : body.dump();
        auto raw = request(method, target, body_str, headers);

        try {
            return nlohmann::json::parse(raw.begin(), raw.end());
        } catch (const nlohmann::json::parse_error&) {
            return nlohmann::json{};
        }
    }

private:
    boost::asio::local::stream_protocol::socket m_socket;
};

// ------------------------------------------------------------
// Docker API operations: pull, create, start, wait.
// ------------------------------------------------------------
[[nodiscard]] bool docker_ping(http_client& client) {
    const auto raw_response = client.request("GET", "/_ping");
    const auto response = nova::data_view{ raw_response };
    nova::topic_log::debug("baldr", "Response: {}", response.as_string());
    return response.as_string() == "OK";
}

void docker_pull(http_client& client, std::string_view image) {
    auto path = fmt::format("{}/images/create?fromImage={}", api_prefix(), image);
    auto resp = client.request_json("POST", path);
    nova::topic_log::info("baldr", "Pull response: {}", resp.dump());
}

[[nodiscard]] std::string docker_create(http_client& client,
                                        std::string_view image,
                                        const std::vector<std::string>& cmd) {
    nlohmann::json body = {
        {"Image", image},
        {"Cmd", cmd},
        {"AttachStdout", true},
        {"AttachStderr", true},
        // {"Tty", true},
        {"HostConfig", { {"AutoRemove", true} }}
    };

    auto resp = client.request_json("POST", api_prefix() + "/containers/create", body);
    if (!resp.contains("Id")) {
        throw nova::exception("Container create failed: {}", resp.dump());
    }
    return resp["Id"].get<std::string>();
}

void docker_start(http_client& client, const std::string& id) {
    const auto raw_response = client.request("POST", fmt::format("{}/containers/{}/start", api_prefix(), id));
    const auto response = nova::data_view{ raw_response };
    nova::topic_log::debug("baldr", "Docker start response: {}", response.as_string());
    nova::topic_log::info("baldr", "Started container: {}", id);
}

[[nodiscard]] int docker_wait(http_client& client, const std::string& id) {
    const auto response = client.request_json("POST", fmt::format("{}/containers/{}/wait", api_prefix(), id));
    nova::topic_log::debug("baldr", "Docker wait response: {}", response.dump());
    if (!response.contains("StatusCode")) {
        return 125;
    }
    return response["StatusCode"].get<int>();
}

void docker_logs(http_client& client, const std::string& id) {
    // The logs API: stream both stdout and stderr
    auto path = fmt::format(
        "{}/containers/{}/logs?stdout=1&stderr=1&follow=1",
        api_prefix(),
        id
    );

    auto raw = client.request("GET", path);
    const auto log_stream = nova::data_view{ raw };
    nova::topic_log::debug("baldr", "Log response: {}", log_stream);

    std::size_t offset = 0;
    while (offset < log_stream.size()) {
        const auto stream_type = log_stream.as_number<std::uint8_t>(offset);
        const auto stream_size = log_stream.as_number<std::uint32_t>(offset + 4);
        auto content = log_stream.as_string(offset + 8, stream_size);
        if (content.ends_with('\n')) {
            content.remove_suffix(1);
        }

        nova::topic_log::debug(
            "baldr",
            "Log stream: type={} size={} content={}",
            stream_type,
            stream_size,
            content
        );

        offset += stream_size + 8;
    }
}


auto parse_args(int argc, char* argv[]) -> std::optional<boost::program_options::variables_map> {
    auto arg_parser = po::options_description("Baldr");

    arg_parser.add_options()
        ("image,i", po::value<std::string>()->required(), "Name of the Docker image")
        ("cmd", po::value<std::vector<std::string>>(), "Commands given to the container")
        ("socket", po::value<std::string>()->default_value(DockerSock), "Docker socker")
        ("help,h", "Show this help message")
    ;

    po::positional_options_description pos;
    pos.add("cmd", -1);

    po::variables_map args;
    auto parsed = po::command_line_parser(argc, argv)
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

    return args;
}

auto entrypoint([[maybe_unused]] const po::variables_map& args) -> int {
    nova::log::load_env_levels();
    nova::log::init("baldr");

    const auto image = args["image"].as<std::string>();
    const auto cmd = args["cmd"].as<std::vector<std::string>>();
    const auto docker_socket = args["socket"].as<std::string>();

    boost::asio::io_context io;
    http_client client{io};
    client.connect(docker_socket);

    if (!docker_ping(client)) {
        nova::topic_log::error("baldr", "Docker daemon not responding");
        return 1;
    }

    // docker_pull(client, image);
    const auto id = docker_create(client, image, cmd);
    docker_start(client, id);
    docker_logs(client, id);
    const int code = docker_wait(client, id);

    return code;
}

MAIN_ARG_PARSE(entrypoint, parse_args);
