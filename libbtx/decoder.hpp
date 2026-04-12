#pragma once

#include <libbtx/descriptor.hpp>
#include <libnova/data.hpp>
#include <libnova/expected.hpp>
#include <libnova/error.hpp>
#include <ostream>

namespace btx {

/**
 * @brief Decode binary data to structured message_data based on the descriptor.
 * @param desc Descriptor schema.
 * @param data Binary data view.
 * @return Parsed message_data or error.
 */
auto decode(const descriptor& desc, nova::data_view data) -> nova::expected<message_data, nova::error>;

/**
 * @brief Decode binary data to annotated BTX text using the descriptor.
 * @param desc Descriptor schema.
 * @param data Binary data view.
 * @return Parsed BTX text or error.
 */
auto format(const descriptor& desc, nova::data_view data) -> nova::expected<nova::bytes, nova::error>;

} // namespace btx
