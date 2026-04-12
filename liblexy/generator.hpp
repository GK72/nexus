/**
 * Part of BTX Toolset.
 *
 * Message generator.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-11
 */

#pragma once

#include <liblexy/descriptor.hpp>
#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <string>
#include <vector>

namespace lexy {

/**
 * @brief   Generate binary data from message values (YAML).
 *
 * @param   desc            Descriptor schema.
 * @param   yaml_values     Message values (YAML).
 *
 * @return  Binary data or error.
 */
[[nodiscard]] auto from_yaml(const descriptor& desc, const std::string& yaml_values) -> nova::expected<std::vector<std::byte>, nova::error>;

/**
 * @brief   Generate random binary data based on the descriptor.
 *
 * @param   desc            Descriptor schema.
 *
 * @return  Binary data or error.
 */
[[nodiscard]] auto generate_random(const descriptor& desc) -> nova::expected<std::vector<std::byte>, nova::error>;

} // namespace lexy
