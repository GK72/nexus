/**
 * Part of Nexus DGM (Diagram as Code).
 *
 * Unit tests for parsing, layout and SVG rendering. Test IDs match
 * `doc/dgm/test-specification.md`.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @date        2026-10-05
 */

#include <dgm/diagram.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace {

/**
 * @brief   Parse and lay out `source` in one step.
 */
[[nodiscard]] auto build(std::string_view source) -> nova::expected<dgm::diagram, nova::error> {
    return dgm::parse(source).and_then([](const dgm::spec& s) { return dgm::layout(s); });
}

/**
 * @brief   Error message of a failed build, or a marker if it unexpectedly succeeded.
 */
[[nodiscard]] auto error_of(std::string_view source) -> std::string {
    const auto res = build(source);
    return res.has_value() ? std::string{ "<no error>" } : res.error().message;
}

constexpr auto InputTC_007 = R"dgm(
// leading comment

box a "Say \"hi\" // not a comment"   at 0,0   // trailing comment
	box b at 200,0
)dgm";

constexpr auto InputTC_010 = R"dgm(
box a "A & <B>" at 0,0
box b           at 200,0
arrow a -> b "x"
)dgm";

TEST(Dgm, TC_001_Positive_AbsolutePlacementAndDefaults) {
    const auto res = build("box a at 10,20\n");
    ASSERT_TRUE(res.has_value()) << res.error().message;
    ASSERT_EQ(res->boxes.size(), 1U);

    EXPECT_EQ(res->boxes[0].id, "a");
    EXPECT_EQ(res->boxes[0].label, "a");
    EXPECT_EQ(res->boxes[0].bounds, (dgm::rect { .x = 10, .y = 20, .width = 120, .height = 50 }));
}

TEST(Dgm, TC_002_Positive_RightOfDefaultGapCentred) {
    const auto res = build(
        "box a at 0,0 size 100x60\n"
        "box b right-of a size 80x20\n"
    );
    ASSERT_TRUE(res.has_value()) << res.error().message;

    EXPECT_EQ(res->boxes[1].bounds, (dgm::rect { .x = 140, .y = 20, .width = 80, .height = 20 }));
}

TEST(Dgm, TC_003_Positive_AllDirectionsWithGap) {
    const auto res = build(
        "box c at 0,0 size 100x50\n"
        "box l left-of c gap 10 size 40x50\n"
        "box u above c gap 20 size 100x30\n"
        "box d below c gap 0 size 60x50\n"
    );
    ASSERT_TRUE(res.has_value()) << res.error().message;

    EXPECT_EQ(res->boxes[1].bounds, (dgm::rect { .x = -50, .y = 0,   .width = 40,  .height = 50 }));
    EXPECT_EQ(res->boxes[2].bounds, (dgm::rect { .x = 0,   .y = -50, .width = 100, .height = 30 }));
    EXPECT_EQ(res->boxes[3].bounds, (dgm::rect { .x = 20,  .y = 50,  .width = 60,  .height = 50 }));
}

TEST(Dgm, TC_004_Positive_FlowPlacement) {
    const auto res = build(
        "box a\n"
        "box b\n"
        "box c below a\n"
        "box d\n"
    );
    ASSERT_TRUE(res.has_value()) << res.error().message;

    EXPECT_EQ(res->boxes[0].bounds.x, 0);
    EXPECT_EQ(res->boxes[0].bounds.y, 0);
    EXPECT_EQ(res->boxes[1].bounds.x, 160);
    EXPECT_EQ(res->boxes[1].bounds.y, 0);
    EXPECT_EQ(res->boxes[3].bounds.x, 160);
    EXPECT_EQ(res->boxes[3].bounds.y, 90);
}

TEST(Dgm, TC_005_Positive_CentringRoundsTowardsTopLeft) {
    const auto res = build(
        "box a at 0,0 size 120x50\n"
        "box b right-of a size 10x41\n"
        "box c left-of a size 10x61\n"
    );
    ASSERT_TRUE(res.has_value()) << res.error().message;

    EXPECT_EQ(res->boxes[1].bounds.y, 4);
    EXPECT_EQ(res->boxes[2].bounds.y, -6);
}

TEST(Dgm, TC_006_Positive_NegativeCoordinatesAndOptionOrder) {
    const auto res = build("box a size 30x40 at -15,-25\n");
    ASSERT_TRUE(res.has_value()) << res.error().message;

    EXPECT_EQ(res->boxes[0].bounds, (dgm::rect { .x = -15, .y = -25, .width = 30, .height = 40 }));
}

TEST(Dgm, TC_007_Positive_CommentsBlankLinesAndEscapes) {
    const auto res = build(InputTC_007);
    ASSERT_TRUE(res.has_value()) << res.error().message;
    ASSERT_EQ(res->boxes.size(), 2U);

    EXPECT_EQ(res->boxes[0].label, "Say \"hi\" // not a comment");
    EXPECT_EQ(res->boxes[1].bounds.x, 200);
}

TEST(Dgm, TC_008_Positive_ArrowsClippedToEdges) {
    const auto res = build(
        "box a at 0,0     size 100x50\n"
        "box b at 200,0   size 100x50\n"
        "box c at 200,200 size 100x50\n"
        "arrow a -> b\n"
        "arrow a -> c\n"
    );
    ASSERT_TRUE(res.has_value()) << res.error().message;
    ASSERT_EQ(res->arrows.size(), 2U);

    const auto& straight = res->arrows[0];
    EXPECT_DOUBLE_EQ(straight.start.x, 100.0);
    EXPECT_DOUBLE_EQ(straight.start.y, 25.0);
    EXPECT_DOUBLE_EQ(straight.end.x, 200.0);
    EXPECT_DOUBLE_EQ(straight.end.y, 25.0);

    const auto& diagonal = res->arrows[1];
    EXPECT_DOUBLE_EQ(diagonal.start.x, 75.0);
    EXPECT_DOUBLE_EQ(diagonal.start.y, 50.0);
    EXPECT_DOUBLE_EQ(diagonal.end.x, 225.0);
    EXPECT_DOUBLE_EQ(diagonal.end.y, 200.0);
}

TEST(Dgm, TC_009_Positive_ArrowForwardReference) {
    const auto res = build(
        "arrow a -> b \"later\"\n"
        "box a at 0,0\n"
        "box b at 200,0\n"
    );
    ASSERT_TRUE(res.has_value()) << res.error().message;
    ASSERT_EQ(res->arrows.size(), 1U);

    EXPECT_EQ(res->arrows[0].label, "later");
}

TEST(Dgm, TC_010_Positive_SvgOutput) {
    const auto res = build(InputTC_010);
    ASSERT_TRUE(res.has_value()) << res.error().message;

    const auto svg = dgm::to_svg(*res);

    EXPECT_TRUE(svg.starts_with("<svg xmlns=\"http://www.w3.org/2000/svg\""));
    EXPECT_NE(svg.find("viewBox=\"-20 -20 360 90\""), std::string::npos);
    EXPECT_NE(svg.find(">A &amp; &lt;B&gt;</text>"), std::string::npos);
    EXPECT_NE(svg.find("x1=\"120\" y1=\"25\" x2=\"200\" y2=\"25\""), std::string::npos);
    EXPECT_NE(svg.find("marker-end=\"url(#dgm-arrowhead)\""), std::string::npos);
    EXPECT_TRUE(svg.ends_with("</svg>\n"));
}

TEST(Dgm, TC_011_Negative_UnknownStatement) {
    EXPECT_EQ(error_of("box a\ncircle b\n"), "line 2: unknown statement 'circle'");
}

TEST(Dgm, TC_012_Negative_InvalidBoxId) {
    EXPECT_EQ(error_of("box\n"), "line 1: invalid box id ''");
    EXPECT_EQ(error_of("box \"a\"\n"), "line 1: invalid box id 'a'");
    EXPECT_EQ(error_of("box a.b\n"), "line 1: invalid box id 'a.b'");
}

TEST(Dgm, TC_013_Negative_DuplicateId) {
    EXPECT_EQ(error_of("box a\nbox a\n"), "line 2: duplicate box id 'a'");
}

TEST(Dgm, TC_014_Negative_InvalidPosition) {
    EXPECT_EQ(error_of("box a at 10\n"), "line 1: invalid position '10', expected X,Y");
    EXPECT_EQ(error_of("box a at 10,y\n"), "line 1: invalid position '10,y', expected X,Y");
    EXPECT_EQ(error_of("box a at\n"), "line 1: invalid position '', expected X,Y");
}

TEST(Dgm, TC_015_Negative_InvalidSize) {
    EXPECT_EQ(error_of("box a size 0x10\n"), "line 1: invalid size '0x10', expected WxH");
    EXPECT_EQ(error_of("box a size 10by10\n"), "line 1: invalid size '10by10', expected WxH");
}

TEST(Dgm, TC_016_Negative_InvalidGap) {
    EXPECT_EQ(error_of("box a\nbox b below a gap -5\n"), "line 2: invalid gap '-5'");
    EXPECT_EQ(error_of("box a\nbox b below a gap\n"), "line 2: invalid gap ''");
}

TEST(Dgm, TC_017_Negative_RelativeToUndeclaredBox) {
    EXPECT_EQ(error_of("box a right-of b\nbox b\n"), "line 1: unknown box 'b' (must be declared before use)");
}

TEST(Dgm, TC_018_Negative_RepeatedOption) {
    EXPECT_EQ(error_of("box a\nbox b at 0,0 below a\n"), "line 2: placement given more than once");
    EXPECT_EQ(error_of("box a size 1x1 size 2x2\n"), "line 1: size given more than once");
}

TEST(Dgm, TC_019_Negative_ArrowMissingOperator) {
    EXPECT_EQ(error_of("box a\nbox b\narrow a b\n"), "line 3: expected '->' in arrow");
}

TEST(Dgm, TC_020_Negative_ArrowUnknownBox) {
    EXPECT_EQ(error_of("box a\narrow a -> z\n"), "line 2: arrow refers to unknown box 'z'");
}

TEST(Dgm, TC_021_Negative_SelfArrow) {
    EXPECT_EQ(error_of("box a\narrow a -> a\n"), "line 2: arrow from 'a' to itself is not supported");
}

TEST(Dgm, TC_022_Negative_UnterminatedString) {
    EXPECT_EQ(error_of("box a \"oops\n"), "line 1: unterminated string");
}

TEST(Dgm, TC_023_Negative_UnexpectedToken) {
    EXPECT_EQ(error_of("box a at 0,0 \"late label\"\n"), "line 1: unexpected token 'late label'");
    EXPECT_EQ(error_of("box a\nbox b\narrow a -> b \"x\" extra\n"), "line 3: unexpected token 'extra'");
}

} // namespace
