#include <libbtx/descriptor.hpp>
#include <yaml-cpp/yaml.h>

namespace btx {

namespace {

/**
 * @brief Parse a single field from a YAML node.
 */
auto parse_field(const YAML::Node& node) -> nova::expected<descriptor::field, nova::error> {
    descriptor::field f;
    f.name = node["name"].as<std::string>();

    std::string type_str = node["type"].as<std::string>("uint");
    if (type_str == "uint") {
        f.type = descriptor::field_type::unsigned_integer;
    } else if (type_str == "bool") {
        f.type = descriptor::field_type::boolean;
    } else if (type_str == "string") {
        f.type = descriptor::field_type::string;
    } else {
        return nova::unexpected(nova::error("Unknown field type: " + type_str));
    }

    std::string len_type_str = node["length_type"].as<std::string>("byte");
    if (len_type_str == "bit") {
        f.len_type = descriptor::length_type::bit;
    } else if (len_type_str == "byte") {
        f.len_type = descriptor::length_type::byte;
    } else {
        return nova::unexpected(nova::error("Unknown length type: " + len_type_str));
    }

    auto len_node = node["length"];
    if (len_node.IsScalar()) {
        try {
            f.length = len_node.as<std::size_t>();
        } catch (...) {
            f.length = len_node.as<std::string>();
        }
    }

    return f;
}

/**
 * @brief Parse the descriptor from a YAML node.
 */
auto parse_descriptor(const YAML::Node& config) -> nova::expected<descriptor, nova::error> {
    try {
        descriptor desc;
        desc.name = config["name"].as<std::string>();
        desc.version = config["version"].as<std::string>("");

        if (config["id"]) {
            if (config["id"].as<std::string>().starts_with("0x")) {
                desc.id = static_cast<uint32_t>(std::stoul(config["id"].as<std::string>(), nullptr, 16));
            } else {
                desc.id = config["id"].as<uint32_t>();
            }
        }

        auto msg_node = config["message"];
        desc.message.name = msg_node["name"].as<std::string>();

        for (auto field_node : msg_node["fields"]) {
            auto f_res = parse_field(field_node);
            if (not f_res) {
                return nova::unexpected(f_res.error());
            }
            desc.message.fields.push_back(*f_res);
        }

        return desc;
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

} // namespace

auto load_descriptor(std::string_view yaml_content) -> nova::expected<descriptor, nova::error> {
    try {
        YAML::Node config = YAML::Load(std::string(yaml_content));
        return parse_descriptor(config);
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

auto load_descriptor_from_file(const std::string& yaml_path) -> nova::expected<descriptor, nova::error> {
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        return parse_descriptor(config);
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

} // namespace btx
