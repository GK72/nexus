#pragma once

#include <nova/color.h>
#include <nova/vec.h>

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace pt {

class image {
public:
    image(nova::Vec2f dimensions)
        : m_dimensions(dimensions)
        , m_data(
            static_cast<std::size_t>(area(dimensions)),
            nova::pack32LE(nova::colors::black)
        )
    {}

    image(const image&) = delete;
    image(image&&) = delete;
    image& operator=(const image&) = delete;
    image& operator=(image&&) = delete;

    [[nodiscard]] const auto* data() const noexcept { return m_data.data(); }
    [[nodiscard]] auto* data()             noexcept { return m_data.data(); }
    [[nodiscard]] auto width()       const noexcept { return m_dimensions.x(); }
    [[nodiscard]] auto height()      const noexcept { return m_dimensions.y(); }
    [[nodiscard]] auto size()        const noexcept { return m_data.size(); }
    [[nodiscard]] const auto& vec()  const noexcept { return m_data; }
    [[nodiscard]] auto dimensions()  const noexcept { return m_dimensions; }
    [[nodiscard]] auto& texture_id()       noexcept { return m_texture_id; }

    [[nodiscard]] auto& at(int x, int y) noexcept {
        using SizeT = decltype(m_data)::size_type;
        return m_data[static_cast<SizeT>(y * static_cast<int>(width()) + x)];
    }

private:
    nova::Vec2f m_dimensions;
    std::vector<std::uint32_t> m_data;
    unsigned int m_texture_id;
};

struct ray {
    nova::Vec3f origin;
    nova::Vec3f direction;

    constexpr nova::Vec3f at(float distance) const {
        return origin + direction * distance;
    }
};

struct hit_record {
    nova::Vec3f point;
    nova::Vec3f normal;
    float t;
    // bool front;
};

// inline bool is_front(const ray& ray, const nova::Vec3f& normal) {

// }

class camera {
public:
    camera(nova::Vec2f dimensions) {
        const auto aspect_ratio = dimensions.x() / dimensions.y();
        const auto viewport_height = 2.0F;
        const auto viewport_width = aspect_ratio * viewport_height;

        m_horizontal  = nova::Vec3f{ viewport_width,  0.0F, 0.0F };
        m_vertical    = nova::Vec3f{ 0.0F, viewport_height, 0.0F };
        bottom_left_calc();
    }

    auto& x() { auto& ret = m_origin.x(); bottom_left_calc(); return ret; };
    auto& y() { auto& ret = m_origin.y(); bottom_left_calc(); return ret; };
    auto& z() { auto& ret = m_origin.z(); bottom_left_calc(); return ret; };

    void bottom_left_calc() {
        m_bottom_left = m_origin
            - m_horizontal / 2.0F
            - m_vertical / 2.0F
            - nova::Vec3f{ 0.0F, 0.0F, m_focal_length };
    }

    auto& focal_length() { auto& ret = m_focal_length; bottom_left_calc(); return ret; }

    ray raycast(float u, float v) const noexcept {
        return {
            m_origin,
            m_bottom_left
            + m_horizontal * u
            + m_vertical * v
            - m_origin
        };
    }

private:
    float m_focal_length = 10.0F;

    nova::Vec3f m_origin { 0.0F, 0.0F, m_focal_length };
    nova::Vec3f m_horizontal;
    nova::Vec3f m_vertical;
    nova::Vec3f m_bottom_left;
};

struct sphere {
    nova::Vec3f position;
    float radius;
    nova::Color color;
};

inline std::optional<hit_record> hit(sphere sphere, const ray& r) {
    const auto x = r.origin - sphere.position;
    const auto a = nova::dot(r.direction, r.direction);
    const auto b = nova::dot(x, r.direction) * 2.0F;
    const auto c = nova::dot(x, x) - sphere.radius * sphere.radius;
    const auto discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return std::nullopt;
    }

    hit_record ret;
    ret.t = (-b - std::sqrt(discriminant)) / (2.0F * a);
    ret.point = r.at(ret.t),
    ret.normal = (ret.point - sphere.position) / sphere.radius;

    return ret;
}

using primitive = std::variant<sphere>;

class pathtracer {
public:
    struct config {
        int sampling = 1;
    };

    pathtracer(image& img, const camera& cam, const std::vector<primitive>& primitives)
        : m_image(img)
        , m_cam(cam)
        , m_primitives(primitives)
    {}

    void update();
    auto& config() { return m_config; }

private:
    image& m_image;
    const camera& m_cam;
    const std::vector<primitive>& m_primitives;

    struct config m_config;

    nova::Color sample(int n, int x, int y);
};

} // namespace pt
