#include <gtest/gtest.h>
#include "descriptor.hpp"
#include "btx.hpp"
#include <sstream>
#include <filesystem>
#include <fstream>

namespace {

TEST(DescriptorTest, LoadAndParseSample) {
    // 1. Prepare sample binary data
    std::string btx_content = R"(
\x09                                    // message_length
\b0001_'0011 \b1110_'1000               // version:1, id:0x3e8
\b1_101_'0000                           // is_alive:1, health:80
\x05                                    // name_length:5
\x4e \x65 \x78 \x75 \x73               // name:"Nexus"
)";
    std::stringstream in(btx_content);
    std::stringstream out;
    auto conv_res = btx::to_binary(in, out);
    ASSERT_TRUE(conv_res.has_value());
    
    std::string binary_data = out.str();
    nova::data_view view(binary_data);

    // 2. Ensure descriptor file exists for test
    // Assuming we run from project root, res/descriptor.yaml should be accessible.
    // If not, we'll create a temporary one.
    const std::string desc_path = "res/descriptor.yaml";
    if (!std::filesystem::exists(desc_path)) {
        GTEST_SKIP() << "res/descriptor.yaml not found, skipping test";
    }

    // 3. Load descriptor
    auto desc_res = btx::descriptor::load(desc_path);
    ASSERT_TRUE(desc_res.has_value()) << desc_res.error().message;
    
    // 4. Parse
    auto parse_res = desc_res->parse(view);
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
    std::ofstream ofs("test_invalid_ref.yaml");
    ofs << yaml;
    ofs.close();

    auto desc_res = btx::descriptor::load("test_invalid_ref.yaml");
    ASSERT_TRUE(desc_res.has_value());
    
    std::string bin = "hello";
    auto parse_res = desc_res->parse(nova::data_view(bin));
    EXPECT_FALSE(parse_res.has_value());
    EXPECT_TRUE(parse_res.error().message.find("Length reference not found") != std::string::npos);
    
    std::filesystem::remove("test_invalid_ref.yaml");
}

} // namespace
