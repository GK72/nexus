#include <gtest/gtest.h>
#include <libbtx/btx.hpp>

#include <libnova/data.hpp>

using namespace nova::literals;

TEST(Btx, BasicHex) {
    auto result = btx::to_binary("\\x48\\x65\\x6c\\x6c\\x6f");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(nova::data_view(*result).as_string(), "Hello");
}

TEST(Btx, WhitespaceAndComments) {
    auto result = btx::to_binary(
        "// Greeting\n"
        "\\x48 \\x65 // H e\n"
        "\\x6c\\x6c\\x6f // l l o"
    );
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(nova::data_view(*result).as_string(), "Hello");
}

TEST(Btx, ComplexHex) {
    auto result = btx::to_binary("\\x00\\xFF\\x01\\x10");
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->size(), 4);
    EXPECT_EQ(static_cast<unsigned char>((*result)[0]), 0x00);
    EXPECT_EQ(static_cast<unsigned char>((*result)[1]), 0xFF);
    EXPECT_EQ(static_cast<unsigned char>((*result)[2]), 0x01);
    EXPECT_EQ(static_cast<unsigned char>((*result)[3]), 0x10);
}

TEST(Btx, Bits) {
    auto result = btx::to_binary("\\b1010'1011");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ(static_cast<unsigned char>((*result)[0]), 0xAB);
}

TEST(Btx, BitsAccumulation) {
    auto result = btx::to_binary("\\b1010____ \\b____1011");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ(static_cast<unsigned char>((*result)[0]), 0xAB);
}

TEST(Btx, BitsSeparators) {
    auto result = btx::to_binary("\\b0001'0000 \\b0010'0000 \\b1100'0000");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ(static_cast<unsigned char>((*result)[0]), 0x10);
    EXPECT_EQ(static_cast<unsigned char>((*result)[1]), 0x20);
    EXPECT_EQ(static_cast<unsigned char>((*result)[2]), 0xC0);
}

TEST(Btx, BitsPadding) {
    auto result = btx::to_binary("\\b101_____");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ(static_cast<unsigned char>((*result)[0]), 0xA0); // 10100000
}

TEST(Btx, BitsAndHexInterleaved) {
    auto result = btx::to_binary("\\b11110000 \\xAA \\b00001111");
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ(static_cast<unsigned char>((*result)[0]), 0xF0);
    EXPECT_EQ(static_cast<unsigned char>((*result)[1]), 0xAA);
    EXPECT_EQ(static_cast<unsigned char>((*result)[2]), 0x0F);
}

TEST(Btx, InvalidHex) {
    auto result = btx::to_binary("\\xGG");
    EXPECT_FALSE(result.has_value());
}

TEST(Btx, InvalidComment) {
    auto result = btx::to_binary("/ Invalid comment");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "Invalid comment: expected '//'");
}

TEST(Btx, FromBinary) {
    std::string data = "Hello";
    nova::data_view view(data);
    btx::config cfg;
    cfg.format_output = false;
    auto result = btx::from_binary(view, cfg);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(nova::data_view(*result).as_string(), "\\x48\\x65\\x6C\\x6C\\x6F");
}

TEST(Btx, FromBinaryFormatted) {
    std::vector<uint8_t> data(10, 0x41); // 10 'A's
    nova::data_view view(data);
    btx::config cfg;
    cfg.format_output = true;
    auto result = btx::from_binary(view, cfg);
    ASSERT_TRUE(result.has_value());

    std::string expected;
    for (int i = 0; i < 8; ++i) expected += "\\x41";
    expected += "\n";
    for (int i = 0; i < 2; ++i) expected += "\\x41";

    EXPECT_EQ(*result, nova::data_view(expected).to_vec());
}
