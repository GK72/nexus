#include <libbtx/btx.h>
#include <libbtx/btx.hpp>

#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <cstring>
#include <cstdlib>

extern "C" {

/**
 * @brief Encodes BTX text into binary.
 */
auto btx_encode(const char* input) -> btx_result_t {
    auto res = btx::to_binary(input);
    btx_result_t result = {nullptr, 0, nullptr};
    if (res) {
        result.size = res->size();
        result.data = static_cast<uint8_t*>(std::malloc(result.size));
        if (result.data) {
            std::memcpy(result.data, res->data(), result.size);
        }
    } else {
        result.error = static_cast<char*>(std::malloc(res.error().message.length() + 1));
        if (result.error) {
            std::strcpy(result.error, res.error().message.c_str());
        }
    }
    return result;
}

/**
 * @brief Decodes binary into BTX text.
 */
auto btx_decode(const uint8_t* data, size_t size) -> btx_result_t {
    auto res = btx::from_binary(nova::data_view{reinterpret_cast<const std::byte*>(data), size});
    btx_result_t result = {nullptr, 0, nullptr};
    if (res) {
        // We allocate size + 1 to optionally null-terminate, though 'size' remains correct.
        result.size = res->size();
        result.data = static_cast<uint8_t*>(std::malloc(result.size + 1));
        if (result.data) {
            std::memcpy(result.data, res->data(), result.size);
            result.data[result.size] = '\0';
        }
    } else {
        result.error = static_cast<char*>(std::malloc(res.error().message.length() + 1));
        if (result.error) {
            std::strcpy(result.error, res.error().message.c_str());
        }
    }
    return result;
}

/**
 * @brief Frees the result.
 */
auto btx_free_result(btx_result_t result) -> void {
    if (result.data) {
        std::free(result.data);
    }
    if (result.error) {
        std::free(result.error);
    }
}

}
