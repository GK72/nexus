#include "pathtracer.h"
#include "camera.h"

#include <nova/vec.h>

#include <cmath>
#include <numbers>
#include <vector>

namespace nxs {

void scan(const image& img, const auto& func) {
    for (int y = static_cast<int>(img.height()) - 1; y >= 0 ; --y) {
        for (int x = 0; x < static_cast<int>(img.width()); ++x) {
            func(x, y);
        }
    }
}

auto hit_sphere(const ray& r, const primitive& prim) {
    const auto x = r.origin - prim.position;
    const auto a = nova::dot(r.direction, r.direction);
    const auto b = nova::dot(x, r.direction) * 2.0F;
    const auto c = nova::dot(x, x) - prim.radius * prim.radius;
    const auto discriminant = b * b - 4 * a * c;
    return discriminant > 0;
}

auto ray_color(const ray& r, const std::vector<primitive>& primitives) {
    for (const auto& elem : primitives) {
        if (hit_sphere(r, elem)) {
            return elem.color;
        }
    }

    // TODO: lerp
    const auto t = (nova::unit(r.direction).y() + 1.0F) * 0.5F;
    return nova::Color{ 1.0F, 1.0F, 1.0F, 1.0F } * (1.0F - t)
         + nova::Color{ 0.5F, 0.7F, 1.0F, 1.0F } * t;
}

void pathtracer(image& img, const camera& cam, const std::vector<primitive>& primitives) {
    scan(img, [&](int x, int y) {
        const auto w = img.width();
        const auto h = img.height();

        const auto u = static_cast<float>(x) / (w - 1);
        const auto v = static_cast<float>(y) / (h - 1);

        const auto direction = cam.bottom_left
            + cam.horizontal * u
            + cam.vertical * v
            - cam.origin;

        const auto r = ray{ cam.origin, direction };
        const auto color = ray_color(r, primitives);

        img.at(x, y) = nova::pack32LE(color);
    });
}

} // namespace nxs
