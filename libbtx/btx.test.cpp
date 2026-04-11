#include <gtest/gtest.h>
#include <libbtx/btx.hpp>
#include <sstream>

TEST(btx, BasicHex) {
    std::stringstream input("\\x48\\x65\\x6c\\x6c\\x6f");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(output.str(), "Hello");
}

TEST(btx, WhitespaceAndComments) {
    std::stringstream input(
        "// Greeting\n"
        "\\x48 \\x65 // H e\n"
        "\\x6c\\x6c\\x6f // l l o"
    );
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(output.str(), "Hello");
}

TEST(btx, ComplexHex) {
    std::stringstream input("\\x00\\xFF\\x01\\x10");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());

    std::string str = output.str();
    ASSERT_EQ(str.length(), 4);
    EXPECT_EQ(static_cast<unsigned char>(str[0]), 0x00);
    EXPECT_EQ(static_cast<unsigned char>(str[1]), 0xFF);
    EXPECT_EQ(static_cast<unsigned char>(str[2]), 0x01);
    EXPECT_EQ(static_cast<unsigned char>(str[3]), 0x10);
}

TEST(btx, Bits) {
    std::stringstream input("\\b1010'1011");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    ASSERT_EQ(output.str().length(), 1);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[0]), 0xAB);
}

TEST(btx, BitsAccumulation) {
    std::stringstream input("\\b1010____ \\b____1011");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    ASSERT_EQ(output.str().length(), 1);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[0]), 0xAB);
}

TEST(btx, BitsSeparators) {
    std::stringstream input("\\b0001'0000 \\b0010'0000 \\b1100'0000");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    ASSERT_EQ(output.str().length(), 3);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[0]), 0x10);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[1]), 0x20);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[2]), 0xC0);
}

TEST(btx, BitsPadding) {
    std::stringstream input("\\b101_____");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    ASSERT_EQ(output.str().length(), 1);
    EXPECT_EQ(static_cast<unsigned char>(output.str()[0]), 0xA0); // 10100000
}

TEST(btx, BitsAndHexInterleaved) {
    std::stringstream input("\\b11110000 \\xAA \\b00001111");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_TRUE(result.has_value());
    
    std::string str = output.str();
    ASSERT_EQ(str.length(), 3);
    EXPECT_EQ(static_cast<unsigned char>(str[0]), 0xF0);
    EXPECT_EQ(static_cast<unsigned char>(str[1]), 0xAA);
    EXPECT_EQ(static_cast<unsigned char>(str[2]), 0x0F);
}

TEST(btx, InvalidHex) {
    std::stringstream input("\\xGG");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_FALSE(result.has_value());
}

TEST(btx, InvalidComment) {
    std::stringstream input("/ Invalid comment");
    std::stringstream output;
    auto result = btx::to_binary(input, output);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message, "Invalid comment: expected '//'");
}

TEST(btx, FromBinary) {
    std::string data = "Hello";
    nova::data_view view(data);
    std::stringstream output;
    btx::config cfg;
    cfg.format_output = false;
    auto result = btx::from_binary(view, output, cfg);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(output.str(), "\\x48\\x65\\x6C\\x6C\\x6F");
}

TEST(btx, FromBinaryFormatted) {
    std::vector<uint8_t> data(10, 0x41); // 10 'A's
    nova::data_view view(data);
    std::stringstream output;
    btx::config cfg;
    cfg.format_output = true;
    auto result = btx::from_binary(view, output, cfg);
    EXPECT_TRUE(result.has_value());
    
    std::string expected;
    for (int i = 0; i < 8; ++i) expected += "\\x41";
    expected += "\n";
    for (int i = 0; i < 2; ++i) expected += "\\x41";

    EXPECT_EQ(output.str(), expected);
}
