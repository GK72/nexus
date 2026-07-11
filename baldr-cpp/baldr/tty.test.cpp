#include <baldr/tty.hpp>

#include <gtest/gtest.h>

#include <string>

TEST(tty, VisibleWidth_PlainText) {
    EXPECT_EQ(tty::visible_width("hello"), 5);
}

TEST(tty, VisibleWidth_StripsSingleAnsiSequence) {
    EXPECT_EQ(tty::visible_width("\x1b[1;34mhello"), 5);
}

TEST(tty, VisibleWidth_StripsMultipleAnsiSequences) {
    EXPECT_EQ(tty::visible_width("\x1b[31mred\x1b[0m"), 3);
}

TEST(tty, VisibleWidth_EmptyString) {
    EXPECT_EQ(tty::visible_width(""), 0);
}

TEST(tty, CapVisibleWidth_UnderBudgetReturnsUnchanged) {
    EXPECT_EQ(tty::cap_visible_width("short", 10), "short");
}

TEST(tty, CapVisibleWidth_TruncatesWithEllipsis) {
    std::string capped = tty::cap_visible_width("abcdefghij", 5);
    EXPECT_EQ(tty::visible_width(capped), 5);
    EXPECT_EQ(capped.substr(capped.size() - 3), "...");
}

TEST(tty, CapVisibleWidth_PreservesTrailingAnsiReset) {
    std::string line = "\x1b[31m" + std::string(20, 'x') + "\x1b[0m";
    std::string capped = tty::cap_visible_width(line, 10);
    EXPECT_EQ(capped.substr(capped.size() - 4), "\x1b[0m");
}

TEST(tty, CapVisibleWidth_ExactBudgetNotTruncated) {
    std::string line = "abcde";
    EXPECT_EQ(tty::cap_visible_width(line, 5), line);
}

TEST(tty, TerminalWidth_FallsBackWhenNotATty) {
    EXPECT_GT(tty::terminal_width(), 0);
}
