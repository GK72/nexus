#pragma once

#include "nxs/utils.h"

#include <cmath>
#include <compare>

namespace nxs {

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

    operator uint32_t() const noexcept { return value; }
};

namespace colors {
    constexpr auto black = vec4{   0.0F,   0.0F,   0.0F, 255.0F };
    constexpr auto white = vec4{ 255.0F, 255.0F, 255.0F, 255.0F };
}

} // namespace nxs
