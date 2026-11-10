/**
 * Part of Nexus Library
 *
 * @brief   Tests of the styling and text layout helpers.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/style.hpp"

#include <gtest/gtest.h>

TEST(Style, DisabledLeavesTextPlain) {
    const auto style = heimdall::style_t::detect(heimdall::color_mode::never);
    EXPECT_EQ(style("text", "red", true), "text");
}

TEST(Style, EnabledPaintsAndKeepsWidth) {
    const heimdall::style_t style { .enabled = true, .truecolor = true };
    const auto painted = style("↑3", "orange", true);
    EXPECT_EQ(painted, "\x1b[1;38;2;245;169;127m↑3\x1b[0m");
    EXPECT_EQ(heimdall::visible_width(painted), 2);
}

TEST(Style, EnabledUsesTheCubeWithoutTruecolor) {
    const heimdall::style_t style { .enabled = true, .truecolor = false };
    EXPECT_EQ(style("x", "red"), "\x1b[38;5;210mx\x1b[0m");
}

TEST(Style, PadCountsCharacters) {
    EXPECT_EQ(heimdall::pad("≡1", 4), "≡1  ");
    EXPECT_EQ(heimdall::pad("long", 2), "long");
}

TEST(Style, Ellipsize) {
    EXPECT_EQ(heimdall::ellipsize("short", 10), "short");
    EXPECT_EQ(heimdall::ellipsize("u/chris/feature", 8), "u/chris…");
}

TEST(Style, Ago) {
    EXPECT_EQ(heimdall::ago(-5), "0s");
    EXPECT_EQ(heimdall::ago(45), "45s");
    EXPECT_EQ(heimdall::ago(12 * 60), "12m");
    EXPECT_EQ(heimdall::ago(21 * 3600), "21h");
    EXPECT_EQ(heimdall::ago(10 * 86400), "10d");
    EXPECT_EQ(heimdall::ago(3 * 604800), "3w");
    EXPECT_EQ(heimdall::ago(5 * 2592000), "5mo");
    EXPECT_EQ(heimdall::ago(2 * 31536000), "2y");
}

TEST(Style, Join) {
    const std::vector<std::string> items { "a", "", "b" };
    EXPECT_EQ(heimdall::join(items, " · "), "a · b");
}
