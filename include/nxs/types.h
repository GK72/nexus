#pragma once

#include <algorithm>
#include <expected>
#include <cmath>
#include <memory_resource>
#include <stdint.h>
#include <vector>

namespace nxs {

template <class T>
concept arithmetic = std::is_arithmetic_v<T>;

template <std::unsigned_integral T>
    requires requires { sizeof(T) == 4; }
[[nodiscard]] constexpr T swapEndian(T x) noexcept {
    return ((x << 24) & 0xFF000000)
         | ((x << 8) & 0x00FF0000)
         | ((x >> 8) & 0x0000FF00)
         | ((x >> 24) & 0x000000FF);
}

/**
 * @brief   Clamp between 0 and 255 for packing four 8-bit values
 */
[[nodiscard]] constexpr uint32_t cast8(arithmetic auto x) noexcept {
    return std::clamp(static_cast<uint32_t>(x), 0u, 255u);
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

struct error {
    std::string msg;
    operator std::string() { return msg; }
};

template <class E> using expected = std::expected<E, error>;
using unexpected = std::unexpected<error>;
using bad_expected_access = std::bad_expected_access<error>;

template <class T> using vector = std::pmr::vector<T>;

struct vec2norm;

struct vec2 {
    float x;
    float y;

    vec2() = default;

    vec2(float x_, float y_)
        : x(x_)
        , y(y_)
    {}

    vec2(vec2norm rhs);

    auto operator<=>(const vec2&) const = default;
};

inline vec2 operator+(const vec2& lhs, const vec2& rhs) noexcept {
    return {
        lhs.x + rhs.x,
        lhs.y + rhs.y
    };
}

inline vec2 operator/(const vec2& lhs, const vec2& rhs) {
    return {
        lhs.x / rhs.x,
        lhs.y / rhs.y
    };
}

[[nodiscard]] inline float norm(vec2 vec) {
    const auto ret = std::sqrt(vec.x * vec.x + vec.y * vec.y);
    return ret == 0 ? 1 : ret;
}

struct vec2norm {
    float x;
    float y;

    vec2norm(float x_, float y_)
        : vec2norm(vec2{ x_, y_ })
    {}

    vec2norm(vec2 rhs)
        : x(rhs.x / norm(rhs))
        , y(rhs.y / norm(rhs))
    {}
};

inline vec2::vec2(vec2norm rhs)
    : x(rhs.x)
    , y(rhs.y)
{}

struct vec4 {
    float x;
    float y;
    float z;
    float w;
};

struct color {
    uint32_t value;

    color(const vec4& rhs)
        : value(pack32LE(rhs.x, rhs.y, rhs.z, rhs.w))
    {}

    operator uint32_t() noexcept { return value; }
};

struct vertex {
    float x;
    float y;
};

namespace colors {
    constexpr auto black = vec4{   0.0f,   0.0f,   0.0f, 255.0f };
    constexpr auto white = vec4{ 255.0f, 255.0f, 255.0f, 255.0f };
}

} // namespace nxs

namespace abyss::engine {

using uint = std::uint32_t;

struct polygon {
    nxs::vector<nxs::vertex> triangles;
    nxs::vector<uint> indicies;
};

} // namespace abyss::engine
