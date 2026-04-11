#pragma once

#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <libnova/data.hpp>

#include <string>
#include <variant>
#include <vector>

/**
 * @namespace btx
 * @brief Binary Text Format library.
 */
namespace btx {

/**
 * @struct field_value
 * @brief Represents a single parsed field value.
 */
struct field_value {
    std::string name;
    std::variant<uint64_t, bool, std::string> value;
};

/**
 * @struct message_data
 * @brief Represents a fully parsed message with its fields.
 */
struct message_data {
    std::string name;
    std::vector<field_value> fields;
};

/**
 * @struct descriptor
 * @brief Schema definition for a message.
 */
struct descriptor {
    /**
     * @brief Represents the length of a field.
     * Can be a fixed number of units (std::size_t) or a reference to another field's value (std::string).
     */
    using field_length = std::variant<std::size_t, std::string>;

    /**
     * @enum field_type
     * @brief Supported field types in the descriptor.
     */
    enum class field_type {
        unsigned_integer,
        boolean,
        string
    };

    /**
     * @enum length_type
     * @brief Units for length specification.
     */
    enum class length_type {
        bit,
        byte
    };

    /**
     * @struct field
     * @brief Schema definition for a single field.
     */
    struct field {
        std::string name;
        field_type type;
        length_type len_type;
        field_length length;
    };

    /**
     * @struct message_schema
     * @brief Schema definition for a message.
     */
    struct message_schema {
        std::string name;
        std::vector<field> fields;
    };

    std::string name;
    std::string version;
    std::uint32_t id;
    message_schema message;

};

/**
 * @brief Load a descriptor from a YAML string.
 * @param yaml_content YAML content.
 * @return btx::descriptor instance or error.
 */
auto load_descriptor(std::string_view yaml_content) -> nova::expected<descriptor, nova::error>;

/**
 * @brief Load a descriptor from a YAML file.
 * @param yaml_path Path to the descriptor.yaml.
 * @return btx::descriptor instance or error.
 */
auto load_descriptor_from_file(const std::string& yaml_path) -> nova::expected<descriptor, nova::error>;

} // namespace btx
