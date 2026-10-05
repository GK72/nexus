/**
 * Part of Nexus DGM (Diagram as Code).
 *
 * Data model: the parsed `spec` with unresolved placements, and the laid out
 * `diagram` with absolute geometry.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @date        2026-10-05
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace dgm {

/**
 * @brief   Default box size in pixels, used when `size` is not given.
 */
constexpr int DefaultBoxWidth = 120;
constexpr int DefaultBoxHeight = 50;

/**
 * @brief   Default distance in pixels between a box and its reference box.
 */
constexpr int DefaultGap = 40;

/**
 * @brief   Integer width and height of a box.
 */
struct extent {
    int width { DefaultBoxWidth };
    int height { DefaultBoxHeight };

    auto operator==(const extent&) const -> bool = default;
};

/**
 * @brief   Placement of the top-left corner at fixed coordinates.
 */
struct absolute {
    int x { 0 };
    int y { 0 };

    auto operator==(const absolute&) const -> bool = default;
};

/**
 * @brief   Side of the reference box a relatively placed box goes to.
 */
enum class direction {
    right_of,
    left_of,
    above,
    below
};

/**
 * @brief   Placement next to an earlier box, centred on the cross axis.
 */
struct relative {
    direction side { direction::right_of };
    std::string anchor {};
    int gap { DefaultGap };

    auto operator==(const relative&) const -> bool = default;
};

/**
 * @brief   No placement given: follow the previous box in a row.
 */
struct flow {
    auto operator==(const flow&) const -> bool = default;
};

/**
 * @brief   Where a box should go, before layout resolves it.
 */
using placement = std::variant<flow, absolute, relative>;

/**
 * @brief   A box as written in the source.
 */
struct box_spec {
    std::string id {};
    std::string label {};
    placement place { flow{} };
    extent size {};
    int line { 0 };
};

/**
 * @brief   An arrow as written in the source.
 */
struct arrow_spec {
    std::string from {};
    std::string to {};
    std::string label {};
    int line { 0 };
};

/**
 * @brief   Parsed source: boxes in declaration order, then arrows.
 */
struct spec {
    std::vector<box_spec> boxes {};
    std::vector<arrow_spec> arrows {};
};

/**
 * @brief   Axis-aligned rectangle; `x`, `y` is the top-left corner.
 */
struct rect {
    int x { 0 };
    int y { 0 };
    int width { 0 };
    int height { 0 };

    auto operator==(const rect&) const -> bool = default;
};

/**
 * @brief   A point in SVG space; fractional because arrows are clipped.
 */
struct point {
    double x { 0.0 };
    double y { 0.0 };
};

/**
 * @brief   A box with its final position.
 */
struct box {
    std::string id {};
    std::string label {};
    rect bounds {};
};

/**
 * @brief   An arrow clipped to the edges of the boxes it connects.
 */
struct arrow {
    point start {};
    point end {};
    std::string label {};
};

/**
 * @brief   Fully laid out diagram, ready to render.
 */
struct diagram {
    std::vector<box> boxes {};
    std::vector<arrow> arrows {};
};

/**
 * @brief   Parse DGM source text.
 *
 * @param   source  Contents of a `.dgm` file.
 *
 * @return  The parsed spec, or the first error with its line number.
 */
[[nodiscard]] auto parse(std::string_view source) -> nova::expected<spec, nova::error>;

/**
 * @brief   Resolve every placement to absolute geometry.
 *
 * @param   input   Parsed spec.
 *
 * @return  The laid out diagram, or the first error with its line number.
 */
[[nodiscard]] auto layout(const spec& input) -> nova::expected<diagram, nova::error>;

/**
 * @brief   Render a laid out diagram as a standalone SVG document.
 *
 * @param   input   Laid out diagram.
 *
 * @return  SVG text.
 */
[[nodiscard]] auto to_svg(const diagram& input) -> std::string;

} // namespace dgm
