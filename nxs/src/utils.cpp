#include <cstddef>

/**
 * @brief   An example function for an interface for other languages.
 */
extern "C" std::uint32_t add(std::int32_t lhs, std::int32_t rhs) {
    return lhs + rhs;
}