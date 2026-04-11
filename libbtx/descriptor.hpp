#pragma once

#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <libnova/data.hpp>

#include <ostream>
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
 * @class descriptor
 * @brief Handles loading and parsing of binary data based on a YAML descriptor.
 */
class descriptor {
public:
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

    /**
     * @brief Load a descriptor from a YAML file.
     * @param yaml_path Path to the descriptor.yaml.
     * @return btx::descriptor instance or error.
     */
    static nova::expected<descriptor, nova::error> load(const std::string& yaml_path);

    /**
     * @brief Parse binary data according to the descriptor.
     * @param data Binary data view.
     * @return Parsed message_data or error.
     */
    nova::expected<message_data, nova::error> parse(nova::data_view data) const;

    /**
     * @brief Generate binary data from message values.
     * @param data Message values (YAML).
     * @return Binary data or error.
     */
    nova::expected<std::vector<std::byte>, nova::error> generate(const std::string& yaml_values) const;

    /**
     * @brief Generate random binary data based on the descriptor.
     * @return Binary data or error.
     */
    nova::expected<std::vector<std::byte>, nova::error> generate_random() const;

    /**
     * @brief Convert binary data to annotated BTX text using the descriptor.
     * @param data Binary data view.
     * @param out Output stream for BTX text.
     * @return Success or error.
     */
    nova::expected<int, nova::error> to_btx(nova::data_view data, std::ostream& out) const;

private:
    std::string m_name;
    std::string m_version;
    std::uint32_t m_id;
    message_schema m_message;
};

} // namespace btx
