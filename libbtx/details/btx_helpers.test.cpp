#include <libbtx/details/btx_helpers.hpp>

#include <gtest/gtest.h>

#include <string_view>

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
    nova::bytes bytes;
    bit_accumulator acc(bytes);

    // Add 4 bits, nothing should be flushed yet
    acc.add_bit(1);
    acc.add_bit(0);
    acc.add_bit(1);
    acc.add_bit(0);
    EXPECT_EQ(bytes.size(), 0);

    // Add 4 more bits, should flush 0xAA (10101010)
    acc.add_bit(1);
    acc.add_bit(0);
    acc.add_bit(1);
    acc.add_bit(0);
    ASSERT_EQ(bytes.size(), 1);
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0xAA);

    // Flush remaining (none)
    acc.flush();
    EXPECT_EQ(bytes.size(), 1);

    // Add 3 bits and flush, should pad with zeros: 11000000 (0xC0)
    acc.add_bit(1);
    acc.add_bit(1);
    acc.add_bit(0);
    acc.flush();
    ASSERT_EQ(bytes.size(), 2);
    EXPECT_EQ(static_cast<unsigned char>(bytes[1]), 0xC0);
}

TEST(btx_helpers, ParseHexToken) {
    nova::bytes bytes;
    bit_accumulator acc(bytes);
    std::string_view input("41");

    auto res = acc.add_hex(input);
    EXPECT_TRUE(res.has_value());

    acc.flush();
    ASSERT_EQ(bytes.size(), 1);
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 'A');
}

TEST(btx_helpers, ParseHexToken_Error) {
    nova::bytes bytes;
    bit_accumulator acc(bytes);
    std::string_view input_err("G1");

    auto res_err = acc.add_hex(input_err);
    EXPECT_FALSE(res_err.has_value());
}

TEST(btx_helpers, ParseBitToken) {
    nova::bytes bytes;
    bit_accumulator acc(bytes);
    std::string_view input("10101010\\x");

    auto res = acc.add_bits(input);
    EXPECT_TRUE(res.has_value());
    ASSERT_EQ(bytes.size(), 1);
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0xAA);

    // it should be at '\'
    EXPECT_EQ(input[0], '\\');
}

TEST(btx_helpers, SkipComment) {
    std::string_view input("// This is a comment\nNext line");
    auto res = skip_comment(input);
    EXPECT_TRUE(res.has_value());

    EXPECT_EQ(input, "Next line");
}

TEST(btx_helpers, InvalidComment) {
    std::string_view input(" This is not a comment\nNext line");
    auto res = skip_comment(input);
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Invalid comment: expected '//'");
}

} // namespace btx::details::test
