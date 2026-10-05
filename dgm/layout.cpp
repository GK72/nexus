/**
 * Part of Nexus DGM (Diagram as Code).
 *
 * Layout: resolves placements to absolute rectangles in one top-to-bottom
 * pass, then clips arrows to box edges. This is the single place that
 * decides positions.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @date        2026-10-05
 */

#include <dgm/diagram.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <variant>

namespace dgm {

namespace {

/**
 * @brief   Build an error carrying the line number.
 */
template <typename ...Args>
[[nodiscard]] auto fail(int line, fmt::format_string<Args...> fmt, Args&& ...args) -> nova::unexpected<nova::error> {
    return nova::unexpected(nova::error(
        fmt::format("line {}: {}", line, fmt::format(fmt, std::forward<Args>(args)...))
    ));
}

/**
 * @brief   Halve `value`, rounding towards negative infinity.
 *
 * Plain integer division rounds towards zero, which would round towards the
 * bottom/right when the box is larger than its reference. Flooring keeps the
 * documented "towards top/left" rule regardless of sign.
 */
[[nodiscard]] constexpr auto half_floor(int value) -> int {
    return value >= 0 ? value / 2 : -((-value + 1) / 2);
}

/**
 * @brief   Rectangle of size `size` placed on `side` of `anchor`.
 */
[[nodiscard]] auto place_next_to(const rect& anchor, direction side, int gap, extent size) -> rect {
    rect out { .x = 0, .y = 0, .width = size.width, .height = size.height };

    switch (side) {
        case direction::right_of:
            out.x = anchor.x + anchor.width + gap;
            out.y = anchor.y + half_floor(anchor.height - size.height);
            break;
        case direction::left_of:
            out.x = anchor.x - gap - size.width;
            out.y = anchor.y + half_floor(anchor.height - size.height);
            break;
        case direction::below:
            out.x = anchor.x + half_floor(anchor.width - size.width);
            out.y = anchor.y + anchor.height + gap;
            break;
        case direction::above:
            out.x = anchor.x + half_floor(anchor.width - size.width);
            out.y = anchor.y - gap - size.height;
            break;
    }

    return out;
}

/**
 * @brief   Centre point of a rectangle.
 */
[[nodiscard]] auto centre(const rect& r) -> point {
    return point { .x = r.x + r.width / 2.0, .y = r.y + r.height / 2.0 };
}

/**
 * @brief   Point where a ray from the centre of `r` along (`dx`, `dy`) leaves `r`.
 *
 * @return  The centre itself when the direction is zero (coincident boxes).
 */
[[nodiscard]] auto clip_to_edge(const rect& r, double dx, double dy) -> point {
    const auto c = centre(r);
    if (dx == 0.0 and dy == 0.0) {
        return c;
    }

    constexpr auto Inf = std::numeric_limits<double>::infinity();
    const auto tx = dx != 0.0 ? (r.width / 2.0) / std::abs(dx) : Inf;
    const auto ty = dy != 0.0 ? (r.height / 2.0) / std::abs(dy) : Inf;
    const auto t = std::min(tx, ty);

    return point { .x = c.x + dx * t, .y = c.y + dy * t };
}

} // namespace

auto layout(const spec& input) -> nova::expected<diagram, nova::error> {
    diagram out;
    std::unordered_map<std::string, std::size_t> index;

    for (const auto& spec_box : input.boxes) {
        if (index.contains(spec_box.id)) {
            return fail(spec_box.line, "duplicate box id '{}'", spec_box.id);
        }

        rect bounds {};

        if (const auto* abs = std::get_if<absolute>(&spec_box.place)) {
            bounds = rect { .x = abs->x, .y = abs->y, .width = spec_box.size.width, .height = spec_box.size.height };
        } else if (const auto* rel = std::get_if<relative>(&spec_box.place)) {
            const auto it = index.find(rel->anchor);
            if (it == index.end()) {
                return fail(spec_box.line, "unknown box '{}' (must be declared before use)", rel->anchor);
            }
            bounds = place_next_to(out.boxes[it->second].bounds, rel->side, rel->gap, spec_box.size);
        } else if (out.boxes.empty()) {
            bounds = rect { .x = 0, .y = 0, .width = spec_box.size.width, .height = spec_box.size.height };
        } else {
            bounds = place_next_to(out.boxes.back().bounds, direction::right_of, DefaultGap, spec_box.size);
        }

        index.emplace(spec_box.id, out.boxes.size());
        out.boxes.push_back(box { .id = spec_box.id, .label = spec_box.label, .bounds = bounds });
    }

    for (const auto& spec_arrow : input.arrows) {
        const auto from = index.find(spec_arrow.from);
        if (from == index.end()) {
            return fail(spec_arrow.line, "arrow refers to unknown box '{}'", spec_arrow.from);
        }
        const auto to = index.find(spec_arrow.to);
        if (to == index.end()) {
            return fail(spec_arrow.line, "arrow refers to unknown box '{}'", spec_arrow.to);
        }
        if (from->second == to->second) {
            return fail(spec_arrow.line, "arrow from '{}' to itself is not supported", spec_arrow.from);
        }

        const auto& a = out.boxes[from->second].bounds;
        const auto& b = out.boxes[to->second].bounds;
        const auto ca = centre(a);
        const auto cb = centre(b);
        const auto dx = cb.x - ca.x;
        const auto dy = cb.y - ca.y;

        out.arrows.push_back(arrow {
            .start = clip_to_edge(a, dx, dy),
            .end   = clip_to_edge(b, -dx, -dy),
            .label = spec_arrow.label
        });
    }

    return out;
}

} // namespace dgm
