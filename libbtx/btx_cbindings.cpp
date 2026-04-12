#include <libbtx/btx.h>
#include <libbtx/btx.hpp>

#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

namespace {
struct btx_handle {
    nova::expected<nova::bytes, nova::error> result;
};
}

extern "C" {

/**
 * @brief Encodes BTX text into binary.
 */
auto btx_encode(const char* input) -> btx_result_t {
    auto handle = new btx_handle{btx::to_binary(input)};
    btx_result_t result = {nullptr, 0, nullptr, handle};

    const auto& res = handle->result;
    if (res) {
        result.data = reinterpret_cast<const uint8_t*>(res->data());
        result.size = res->size();
    } else {
        result.error = res.error().message.c_str();
    }
    return result;
}

/**
 * @brief Decodes binary into BTX text.
 */
auto btx_decode(const uint8_t* data, size_t size) -> btx_result_t {
    auto handle = new btx_handle{btx::from_binary(nova::data_view{reinterpret_cast<const std::byte*>(data), size})};
    btx_result_t result = {nullptr, 0, nullptr, handle};

    const auto& res = handle->result;
    if (res) {
        result.data = reinterpret_cast<const uint8_t*>(res->data());
        result.size = res->size();
    } else {
        result.error = res.error().message.c_str();
    }
    return result;
}

/**
 * @brief Frees the result.
 */
auto btx_free_result(btx_result_t result) -> void {
    if (result.internal) {
        delete static_cast<btx_handle*>(result.internal);
    }
}

}
