#include "pathtracer.h"
#include "types.h"

#include <nova/random.h>
#include <nova/vec.h>

#include <concepts>
#include <cmath>
#include <numbers>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace nxs {

void scan(const image& img, const auto& func) {
    for (int y = static_cast<int>(img.height()) - 1; y >= 0 ; --y) {
        for (int x = 0; x < static_cast<int>(img.width()); ++x) {
            func(x, y);
        }
    }
}

auto ray_color(const ray& r, const std::vector<primitive>& primitives) {
    for (const auto& elem : primitives) {
        const std::optional<hit_record> ret = std::visit([&r](auto& p) { return hit(p, r); }, elem);
        if (ret.has_value()) {
            return (
                nova::Color{
                    ret->normal.x(),
                    ret->normal.y(),
                    ret->normal.z(),
                    1.0F
                } + 1.0F
            ) * 0.5F;
        }
    }

    // Background

    // TODO: lerp
    const auto t = (nova::unit(r.direction).y() + 1.0F) * 0.5F;
    return nova::Color{ 1.0F, 1.0F, 1.0F, 1.0F } * (1.0F - t)
         + nova::Color{ 0.5F, 0.7F, 1.0F, 1.0F } * t;
}

nova::Color pathtracer::sample(int n, int x, int y) {
    const auto& w = m_image.width();
    const auto& h = m_image.height();

    nova::Color color {};

    float ran_x = n > 1 ? nova::random().number() : 0.0F;
    float ran_y = n > 1 ? nova::random().number() : 0.0F;

    for (int i = 0; i < n; ++i) {
        const auto u = (static_cast<float>(x) + ran_x) / (w - 1);
        const auto v = (static_cast<float>(y) + ran_y) / (h - 1);

        color += ray_color(m_cam.raycast(u, v), m_primitives);
    }

    return color / static_cast<float>(n);
}

void pathtracer::update() {
    scan(m_image, [this](int x, int y) {
        const auto color = sample(m_config.sampling, x, y);
        m_image.at(x, y) = nova::pack32LE(color);
    });
}

} // namespace nxs
