#include <gtest/gtest.h>
#include <libbtx/descriptor.hpp>
#include <libbtx/decoder.hpp>
#include <libbtx/btx.hpp>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace {

TEST(DescriptorTest, LoadAndParseSample) {
    // 1. Prepare sample binary data
    std::string btx_content = R"(
\x09                                    // message_length
\b00010011 \b11101000               // version:1, id:0x3e8
\b11010000                           // is_alive:1, health:80
\x05                                    // name_length:5
\x4e \x65 \x78 \x75 \x73               // name:"Nexus"
)";
    std::stringstream in(btx_content);
    std::stringstream out;
    auto conv_res = btx::to_binary(in, out);
    ASSERT_TRUE(conv_res.has_value());

    std::string binary_data = out.str();
    nova::data_view view(binary_data);

    // 2. Load descriptor
    const std::string desc_path = "res/descriptor.yaml";
    if (!std::filesystem::exists(desc_path)) {
        GTEST_SKIP() << "res/descriptor.yaml not found, skipping test";
    }

    auto desc_res = btx::load_descriptor_from_file(desc_path);
    ASSERT_TRUE(desc_res.has_value()) << desc_res.error().message;

    // 3. Parse
    auto parse_res = btx::decode(*desc_res, view);
    ASSERT_TRUE(parse_res.has_value()) << parse_res.error().message;

    EXPECT_EQ(parse_res->name, "player");
    ASSERT_EQ(parse_res->fields.size(), 7);

    EXPECT_EQ(parse_res->fields[0].name, "message_length");
    EXPECT_EQ(std::get<uint64_t>(parse_res->fields[0].value), 9);

    EXPECT_EQ(parse_res->fields[1].name, "version");
    EXPECT_EQ(std::get<uint64_t>(parse_res->fields[1].value), 1);

    EXPECT_EQ(parse_res->fields[2].name, "id");
    EXPECT_EQ(std::get<uint64_t>(parse_res->fields[2].value), 0x3e8);

    EXPECT_EQ(parse_res->fields[3].name, "is_alive");
    EXPECT_EQ(std::get<bool>(parse_res->fields[3].value), true);

    EXPECT_EQ(parse_res->fields[4].name, "health");
    EXPECT_EQ(std::get<uint64_t>(parse_res->fields[4].value), 80);

    EXPECT_EQ(parse_res->fields[5].name, "name_length");
    EXPECT_EQ(std::get<uint64_t>(parse_res->fields[5].value), 5);

    EXPECT_EQ(parse_res->fields[6].name, "name");
    EXPECT_EQ(std::get<std::string>(parse_res->fields[6].value), "Nexus");
}

TEST(DescriptorTest, InvalidReference) {
    // Descriptor with non-existent reference
    std::string yaml = R"(
name: test
message:
  name: test_msg
  fields:
    - name: data
      type: string
      length: unknown_field
      length_type: byte
)";
    auto desc_res = btx::load_descriptor(yaml);
    ASSERT_TRUE(desc_res.has_value());

    std::string bin = "hello";
    auto parse_res = btx::decode(*desc_res, nova::data_view(bin));
    EXPECT_FALSE(parse_res.has_value());
    EXPECT_TRUE(parse_res.error().message.find("Length reference not found") != std::string::npos);
}

TEST(DescriptorTest, LoadMinimal) {
    std::string yaml = R"(
name: minimal
message:
  name: msg
  fields:
    - name: f1
      type: uint
      length: 1
)";
    auto res = btx::load_descriptor(yaml);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->name, "minimal");
    EXPECT_EQ(res->message.fields.size(), 1);
    EXPECT_EQ(res->message.fields[0].name, "f1");
}

TEST(DescriptorTest, LoadMissingFile) {
    auto res = btx::load_descriptor_from_file("non_existent.yaml");
    EXPECT_FALSE(res.has_value());
}

} // namespace
