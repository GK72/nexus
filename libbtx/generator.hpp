#pragma once

#include <libbtx/descriptor.hpp>
#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <string>
#include <vector>

namespace btx::generator {

/**
 * @brief Generate binary data from message values (YAML).
 * @param desc Descriptor schema.
 * @param yaml_values Message values (YAML).
 * @return Binary data or error.
 */
auto from_yaml(const descriptor& desc, const std::string& yaml_values) -> nova::expected<std::vector<std::byte>, nova::error>;

/**
 * @brief Generate random binary data based on the descriptor.
 * @param desc Descriptor schema.
 * @return Binary data or error.
 */
auto generate_random(const descriptor& desc) -> nova::expected<std::vector<std::byte>, nova::error>;

} // namespace btx::generator
