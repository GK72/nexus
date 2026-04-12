/**
 * Part of BTX Toolset.
 *
 * Message descriptor describing any kind of binary blob.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-11
 */

#pragma once

#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <libnova/data.hpp>

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace lexy {

/**
 * @brief   Substitute for `nova::expected<void, ...>`.
 */
struct empty {};

/**
 * @brief   Represents a single parsed field value.
 */
struct field_value {
    std::string name;
    std::variant<std::uint64_t, bool, std::string> value;
};

/**
 * @brief   Represents a fully parsed message with its fields.
 */
struct message_data {
    std::string name;
    std::vector<field_value> fields;
};

/**
 * @brief   Schema definition for a message.
 */
struct descriptor {

    /**
     * @brief   Represents the length of a field.
     *
     * Can be a fixed number of units (std::size_t) or a reference to another
     * field's value (std::string).
     */
    using field_length = std::variant<std::size_t, std::string>;

    /**
     * @brief   Supported field types in the descriptor.
     */
    enum class field_type {
        unsigned_integer,
        boolean,
        string
    };

    /**
     * @brief   Units for length specification.
     */
    enum class length_type {
        bit,
        byte
    };

    /**
     * @brief   Schema definition for a single field.
     */
    struct field {
        std::string name;
        field_type type;
        length_type len_type;
        field_length length;
    };

    /**
     * @brief   Schema definition for a message.
     */
    struct message_schema {
        std::string name;
        std::uint32_t id;
        std::vector<field> fields;
    };

    std::string name;
    std::string version;
    message_schema message;

};

/**
 * @brief   Load a descriptor from a YAML content string.
 *
 * @param   yaml_content    YAML content.
 *
 * @return  Descriptor instance or error.
 */
[[nodiscard]] auto load_descriptor(std::string_view yaml_content)
        -> nova::expected<descriptor, nova::error>;

[[nodiscard]] inline auto load_descriptor(const char* yaml_content)
        -> nova::expected<descriptor, nova::error>
{
    return load_descriptor(std::string_view(yaml_content));
}

[[nodiscard]] inline auto load_descriptor(const std::string& yaml_content)
        -> nova::expected<descriptor, nova::error>
{
    return load_descriptor(std::string_view(yaml_content));
}

/**
 * @brief   Load a descriptor from a YAML file.
 *
 * @param   yaml_path       Path to the descriptor.yaml.
 *
 * @return  Descriptor instance or error.
 */
[[nodiscard]] auto load_descriptor(const std::filesystem::path& yaml_path) -> nova::expected<descriptor, nova::error>;

} // namespace lexy
