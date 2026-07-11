#include <gtest/gtest.h>
#include <baldr/line_reader.hpp>

#include <string>
#include <vector>

TEST(line_reader, SingleCompleteLine) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("hello\n");

    ASSERT_EQ(received.size(), 1);
    EXPECT_EQ(received[0], "hello");
}

TEST(line_reader, MultipleLinesInOneChunk) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("a\nb\nc\n");

    ASSERT_EQ(received.size(), 3);
    EXPECT_EQ(received[0], "a");
    EXPECT_EQ(received[1], "b");
    EXPECT_EQ(received[2], "c");
}

TEST(line_reader, LineSplitAcrossTwoFeeds) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("hel");
    reader.feed("lo\n");

    ASSERT_EQ(received.size(), 1);
    EXPECT_EQ(received[0], "hello");
}

TEST(line_reader, NoNewlineYieldsNoDispatch) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("partial");

    EXPECT_TRUE(received.empty());
}

TEST(line_reader, FeedEofFlushesTrailingPartialLine) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("partial");
    reader.feed_eof();

    ASSERT_EQ(received.size(), 1);
    EXPECT_EQ(received[0], "partial");
}

TEST(line_reader, FeedEofIsNoOpWhenBufferEmpty) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("line\n");
    reader.feed_eof();

    ASSERT_EQ(received.size(), 1);
    EXPECT_EQ(received[0], "line");
}

TEST(line_reader, FeedEofIsNoOpOnSecondCall) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("partial");
    reader.feed_eof();
    reader.feed_eof();

    ASSERT_EQ(received.size(), 1);
    EXPECT_EQ(received[0], "partial");
}

TEST(line_reader, EmptyLinesArePreserved) {
    std::vector<std::string> received;
    baldr::line_reader reader([&received](std::string& line) { received.push_back(line); });

    reader.feed("a\n\nb\n");

    ASSERT_EQ(received.size(), 3);
    EXPECT_EQ(received[0], "a");
    EXPECT_EQ(received[1], "");
    EXPECT_EQ(received[2], "b");
}
