/**
 * Part of BTX Toolset.
 *
 * Binary decoder into annotated BTX format.
 *
 * @author  Gábor Krisztián Girhiny and Junie
 * @date    2026-04-11
 */

#pragma once

#include <liblexy/descriptor.hpp>

#include <libnova/data.hpp>
#include <libnova/expected.hpp>
#include <libnova/error.hpp>

namespace lexy {

/**
 * @brief   Decode binary data to structured message_data based on the descriptor.
 *
 * @param   desc    Descriptor schema.
 * @param   data    Binary data view.
 *
 * @return  Parsed message_data or error.
 */
[[nodiscard]] auto decode(const descriptor& desc, nova::data_view data) -> nova::expected<message_data, nova::error>;

/**
 * @brief   Decode binary data to annotated BTX text using the descriptor.
 *
 * @param   desc    Descriptor schema.
 * @param   data    Binary data view.
 *
 * @return  Parsed BTX text or error.
 */
[[nodiscard]] auto format(const descriptor& desc, nova::data_view data) -> nova::expected<nova::bytes, nova::error>;

} // namespace lexy
