#pragma once

#include "nxs/types.h"
#include "nxs/type_traits.h"

#include <fmt/format.h>

#include <chrono>
#include <exception>

#ifndef NDEBUG
#define IFDBG(x) x
#else
#define IFDBG(x) ((void)0)
#endif

namespace nxs {

class unreachable : public std::exception {
public:
    const char* what() const noexcept override {
        return "Unhandled enum";
    }
};

struct identity {
    constexpr auto& operator()(auto& x) noexcept {
        return x;
    }
};

[[nodiscard]] inline std::chrono::nanoseconds now() {
    return std::chrono::steady_clock::now().time_since_epoch();
}

[[nodiscard]] inline auto toUs(std::chrono::nanoseconds x) {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(x).count());
}

/**
 * @brief   Clamp between 0 and 255 for packing four 8-bit values
 */
[[nodiscard]] constexpr uint32_t cast8(arithmetic auto x) noexcept {
    return std::clamp(static_cast<uint32_t>(x), 0U, 255U);
}

/**
 * @brief   Pack 4 8-bit values (Little endian)
 */
template <arithmetic T>
[[nodiscard]] constexpr uint32_t pack32LE(T x, T y, T z, T w) noexcept {
    return cast8(w) << 24
         | cast8(z) << 16
         | cast8(y) << 8
         | cast8(x);
}

/**
 * @brief   Pack 4 8-bit values (Big endian)
 */
template <arithmetic T>
[[nodiscard]] constexpr uint32_t pack32BE(T x, T y, T z, T w) noexcept {
    return pack32LE(w, z, y, x);
}

} // namespace nxs

template<>
struct fmt::formatter<nxs::error> {
    template <class ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(const nxs::error& x, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", x.msg);
    }
};
