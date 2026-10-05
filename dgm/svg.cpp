/**
 * Part of Nexus DGM (Diagram as Code).
 *
 * SVG renderer: plain boxes, straight arrows and labels. The canvas is sized
 * to fit every box plus a fixed margin, so negative coordinates just work.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @date        2026-10-05
 */

#include <dgm/diagram.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>

namespace dgm {

namespace {

/**
 * @brief   Empty space around the outermost boxes, in pixels.
 */
constexpr int CanvasMargin = 20;

/**
 * @brief   Vertical offset of an arrow label above the arrow's midpoint.
 */
constexpr double ArrowLabelLift = 6.0;

/**
 * @brief   Escape the five XML special characters.
 */
[[nodiscard]] auto escape_xml(std::string_view text) -> std::string {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;        break;
        }
    }
    return out;
}

/**
 * @brief   Format a coordinate with at most two decimals and no noise.
 */
[[nodiscard]] auto num(double value) -> std::string {
    constexpr double Scale = 100.0;
    const double rounded = std::round(value * Scale) / Scale;
    return fmt::format("{}", rounded == 0.0 ? 0.0 : rounded);
}

/**
 * @brief   Bounding box of all boxes, or an empty rect at the origin.
 */
[[nodiscard]] auto content_bounds(const diagram& input) -> rect {
    if (input.boxes.empty()) {
        return rect {};
    }

    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    int max_y = std::numeric_limits<int>::min();

    for (const auto& b : input.boxes) {
        min_x = std::min(min_x, b.bounds.x);
        min_y = std::min(min_y, b.bounds.y);
        max_x = std::max(max_x, b.bounds.x + b.bounds.width);
        max_y = std::max(max_y, b.bounds.y + b.bounds.height);
    }

    return rect { .x = min_x, .y = min_y, .width = max_x - min_x, .height = max_y - min_y };
}

} // namespace

auto to_svg(const diagram& input) -> std::string {
    const auto content = content_bounds(input);
    const rect view {
        .x      = content.x - CanvasMargin,
        .y      = content.y - CanvasMargin,
        .width  = content.width + 2 * CanvasMargin,
        .height = content.height + 2 * CanvasMargin
    };

    std::string out;
    auto it = std::back_inserter(out);

    fmt::format_to(it,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"{} {} {} {}\" width=\"{}\" height=\"{}\""
        " font-family=\"sans-serif\" font-size=\"14\">\n",
        view.x, view.y, view.width, view.height, view.width, view.height
    );
    fmt::format_to(it,
        "  <defs>\n"
        "    <marker id=\"dgm-arrowhead\" viewBox=\"0 0 10 10\" refX=\"10\" refY=\"5\""
        " markerWidth=\"10\" markerHeight=\"10\" markerUnits=\"userSpaceOnUse\" orient=\"auto\">\n"
        "      <path d=\"M0,0 L10,5 L0,10 z\" fill=\"#333\"/>\n"
        "    </marker>\n"
        "  </defs>\n"
    );
    fmt::format_to(it,
        "  <rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\" fill=\"#fff\"/>\n",
        view.x, view.y, view.width, view.height
    );

    for (const auto& b : input.boxes) {
        const auto& r = b.bounds;
        fmt::format_to(it,
            "  <g id=\"box-{}\">\n"
            "    <rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\" rx=\"4\" fill=\"#fff\" stroke=\"#333\" stroke-width=\"1.5\"/>\n"
            "    <text x=\"{}\" y=\"{}\" text-anchor=\"middle\" dominant-baseline=\"central\" fill=\"#111\">{}</text>\n"
            "  </g>\n",
            escape_xml(b.id),
            r.x, r.y, r.width, r.height,
            num(r.x + r.width / 2.0), num(r.y + r.height / 2.0), escape_xml(b.label)
        );
    }

    for (const auto& a : input.arrows) {
        fmt::format_to(it,
            "  <line x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\" stroke=\"#333\" stroke-width=\"1.5\""
            " marker-end=\"url(#dgm-arrowhead)\"/>\n",
            num(a.start.x), num(a.start.y), num(a.end.x), num(a.end.y)
        );
        if (not a.label.empty()) {
            fmt::format_to(it,
                "  <text x=\"{}\" y=\"{}\" text-anchor=\"middle\" font-size=\"12\" fill=\"#555\""
                " stroke=\"#fff\" stroke-width=\"4\" paint-order=\"stroke\">{}</text>\n",
                num((a.start.x + a.end.x) / 2.0), num((a.start.y + a.end.y) / 2.0 - ArrowLabelLift),
                escape_xml(a.label)
            );
        }
    }

    out += "</svg>\n";
    return out;
}

} // namespace dgm
