/**
 * Part of BTX Toolset.
 *
 * Message descriptor describing any kind of binary blob.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-11
 */

#include <liblexy/descriptor.hpp>

#include <yaml-cpp/yaml.h>

#include <filesystem>

namespace lexy {

namespace {

/**
 * @brief   Parse a single field from a YAML node.
 *
 * @param   node    YAML node.
 *
 * Parses the following nodes out of field:
 * - `type`
 * - `length`
 * - `length_type`
 *
 * @return  Field definition or error.
 */
[[nodiscard]] auto parse_field(const YAML::Node& node)
        -> nova::expected<descriptor::field, nova::error>
{
    descriptor::field field;
    field.name = node["name"].as<std::string>();

    if (not node["type"]) {
        return nova::unexpected(nova::error("Missing mandatory field: type"));
    }

    const std::string type_str = node["type"].as<std::string>();
    if (type_str == "uint") {
        field.type = descriptor::field_type::unsigned_integer;
    } else if (type_str == "bool") {
        field.type = descriptor::field_type::boolean;
    } else if (type_str == "string") {
        field.type = descriptor::field_type::string;
    } else {
        return nova::unexpected(nova::error("Unknown field type: " + type_str));
    }

    const std::string len_type_str = node["length_type"].as<std::string>("byte");
    if (len_type_str == "bit") {
        field.len_type = descriptor::length_type::bit;
    } else if (len_type_str == "byte") {
        field.len_type = descriptor::length_type::byte;
    } else {
        return nova::unexpected(nova::error("Unknown length type: " + len_type_str));
    }

    auto len_node = node["length"];
    if (len_node.IsScalar()) {
        try {
            field.length = len_node.as<std::size_t>();
        } catch (...) {
            field.length = len_node.as<std::string>();
        }
    }

    return field;
}

/**
 * @brief   Parse the descriptor from a YAML node.
 *
 * @param   config  YAML configuration.
*
 * Parses the following nodes out of the descriptor:
 * - `name`
 * - `message.id`
 * - `message.fields`
 *
 * @return  Descriptor instance or error.
*
 * @throws  Unhandled errors.
 */
[[nodiscard]] auto parse_descriptor(const YAML::Node& config)
        -> nova::expected<descriptor, nova::error>
{
    descriptor desc;
    if (not config["name"]) {
        return nova::unexpected(nova::error("Missing mandatory field: name"));
    }

    desc.name = config["name"].as<std::string>();
    desc.version = config["version"].as<std::string>("");

    auto msg_node = config["message"];
    if (not msg_node) {
        return nova::unexpected(nova::error("Missing mandatory field: message"));
    }

    if (not msg_node["id"]) {
        return nova::unexpected(nova::error("Missing mandatory field: message:id"));
    }

    if (msg_node["id"].as<std::string>().starts_with("0x")) {
        desc.message.id = static_cast<std::uint32_t>(std::stoul(msg_node["id"].as<std::string>(), nullptr, 16));
    } else {
        desc.message.id = msg_node["id"].as<std::uint32_t>();
    }

    desc.message.name = msg_node["name"].as<std::string>();

    for (auto field_node : msg_node["fields"]) {
        auto field = parse_field(field_node);

        if (not field) {
            return nova::unexpected(field.error());
        }

        desc.message.fields.push_back(*field);
    }

    return desc;
}

} // namespace

[[nodiscard]] auto load_descriptor(std::string_view yaml_content)
        -> nova::expected<descriptor, nova::error>
{
    try {
        YAML::Node config = YAML::Load(std::string(yaml_content));
        return parse_descriptor(config);
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

[[nodiscard]] auto load_descriptor(const std::filesystem::path& yaml_path)
        -> nova::expected<descriptor, nova::error>
{
    try {
        YAML::Node config = YAML::LoadFile(yaml_path.string());
        return parse_descriptor(config);
    } catch (const std::exception& e) {
        return nova::unexpected(nova::error(e.what()));
    }
}

} // namespace lexy
