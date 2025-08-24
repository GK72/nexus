#include <boost/asio.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

using boost::asio::local::stream_protocol;

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
