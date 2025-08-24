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
