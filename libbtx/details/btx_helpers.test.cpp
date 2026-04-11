#include <libbtx/details/btx_helpers.hpp>

#include <gtest/gtest.h>

#include <sstream>

namespace btx::details::test {

TEST(btx_helpers, HexToInt) {
    EXPECT_EQ(*hex_to_int('0'), 0);
    EXPECT_EQ(*hex_to_int('9'), 9);
    EXPECT_EQ(*hex_to_int('a'), 10);
    EXPECT_EQ(*hex_to_int('f'), 15);
    EXPECT_EQ(*hex_to_int('A'), 10);
    EXPECT_EQ(*hex_to_int('F'), 15);
    EXPECT_FALSE(hex_to_int('g').has_value());
}

TEST(btx_helpers, BitAccumulator) {
    bit_accumulator acc;
    std::stringstream ss;

    // Add 4 bits, nothing should be flushed yet
    acc.add_bit(1, ss);
    acc.add_bit(0, ss);
    acc.add_bit(1, ss);
    acc.add_bit(0, ss);
    EXPECT_EQ(ss.str().length(), 0);

    // Add 4 more bits, should flush 0xAA (10101010)
    acc.add_bit(1, ss);
    acc.add_bit(0, ss);
    acc.add_bit(1, ss);
    acc.add_bit(0, ss);
    ASSERT_EQ(ss.str().length(), 1);
    EXPECT_EQ(static_cast<unsigned char>(ss.str()[0]), 0xAA);

    // Flush remaining (none)
    acc.flush(ss);
    EXPECT_EQ(ss.str().length(), 1);

    // Add 3 bits and flush, should pad with zeros: 11000000 (0xC0)
    acc.add_bit(1, ss);
    acc.add_bit(1, ss);
    acc.add_bit(0, ss);
    acc.flush(ss);
    ASSERT_EQ(ss.str().length(), 2);
    EXPECT_EQ(static_cast<unsigned char>(ss.str()[1]), 0xC0);
}

TEST(btx_helpers, ParseHexToken) {
    bit_accumulator acc;
    std::stringstream input("41");
    std::stringstream output;

    auto res = acc.add_hex_from_stream(input, output);
    EXPECT_TRUE(res.has_value());

    acc.flush(output);
    EXPECT_EQ(output.str(), "A");
}

TEST(btx_helpers, ParseHexToken_Error) {
    bit_accumulator acc;
    std::stringstream input_err("G1");
    std::stringstream output_err;

    auto res_err = acc.add_hex_from_stream(input_err, output_err);
    EXPECT_FALSE(res_err.has_value());
}

TEST(btx_helpers, ParseBitToken) {
    bit_accumulator acc;
    std::stringstream input("10101010\\x");
    std::stringstream output;

    auto res = acc.add_bits_from_stream(input, output);
    EXPECT_TRUE(res.has_value());
    ASSERT_EQ(output.str().length(), 1);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[0]), 0xAA);

    // peek should be at '\'
    EXPECT_EQ(input.peek(), '\\');
}

TEST(btx_helpers, SkipComment) {
    std::stringstream input("// This is a comment\nNext line");
    auto ch = input.get();
    EXPECT_EQ(ch, '/');
    auto res = skip_comment(input);
    EXPECT_TRUE(res.has_value());

    std::string line;
    std::getline(input, line);
    EXPECT_EQ(line, "Next line");
}

TEST(btx_helpers, InvalidComment) {
    std::stringstream input(" This is not a comment\nNext line");
    // ch would be '/' consumed by caller
    auto res = skip_comment(input);
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Invalid comment: expected '//'");
}

} // namespace btx::details::test
