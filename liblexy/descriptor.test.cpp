#include <gtest/gtest.h>

#include <libbtx/btx.hpp>
#include <liblexy/descriptor.hpp>
#include <liblexy/decoder.hpp>

#include <filesystem>
#include <fstream>

namespace {

constexpr auto InputTC_001 = R"yaml(
name: minimal
version: 1.0.0
message:
  name: msg
  id: 0x1234
  fields:
    - name: f1
      type: uint
      length: 1
)yaml";

TEST(BtxDescriptor, TC_001_Positive_FullDescriptorLoading) {
    const auto res = lexy::load_descriptor(InputTC_001);
    ASSERT_TRUE(res.has_value());

    EXPECT_EQ(res->name, "minimal");
    EXPECT_EQ(res->version, "1.0.0");
    EXPECT_EQ(res->message.id, 0x1234);
    EXPECT_EQ(res->message.fields.size(), 1);
    EXPECT_EQ(res->message.fields[0].name, "f1");
    EXPECT_EQ(res->message.fields[0].type, lexy::descriptor::field_type::unsigned_integer);
}


constexpr auto InputTC_002_Hex = R"yaml(
name: test
message:
  name: msg
  id: 0x1234
  fields: []
)yaml";

constexpr auto InputTC_002_Dec = R"yaml(
name: test
message:
  name: msg
  id: 4660
  fields: []
)yaml";

TEST(BtxDescriptor, TC_002_Positive_HexAndDecimalId) {
    const auto res_hex = lexy::load_descriptor(InputTC_002_Hex);
    const auto res_dec = lexy::load_descriptor(InputTC_002_Dec);

    ASSERT_TRUE(res_hex.has_value());
    ASSERT_TRUE(res_dec.has_value());
    EXPECT_EQ(res_hex->message.id, 4660);
    EXPECT_EQ(res_dec->message.id, 4660);
}


constexpr auto InputTC_003 = R"yaml(
name: test
message:
  name: msg
  id: 1
  fields:
    - name: f1
      length: 1
)yaml";

TEST(BtxDescriptor, TC_003_Negative_MissingType) {
    const auto res = lexy::load_descriptor(InputTC_003);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Missing mandatory field: type");
}


constexpr auto InputTC_004 = R"yaml(
name: test
message:
  name: msg
  fields: []
)yaml";


TEST(BtxDescriptor, TC_004_Negative_MissingId) {
    const auto res = lexy::load_descriptor(InputTC_004);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Missing mandatory field: message:id");
}


constexpr auto InputTC_005 = R"yaml(
name: test
message:
  name: msg
  id: 1
  fields:
    - name: f1
      type: float
)yaml";

TEST(BtxDescriptor, TC_005_Negative_UnknownFieldType) {
    const auto res = lexy::load_descriptor(InputTC_005);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Unknown field type: float");
}


constexpr auto InputTC_006 = R"yaml(
name: test
message:
  name: msg
  id: 1
  fields:
    - name: f1
      type: uint
      length: 1
      length_type: kilobyte
)yaml";

TEST(BtxDescriptor, TC_006_Negative_UnknownLengthType) {
    const auto res = lexy::load_descriptor(InputTC_006);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Unknown length type: kilobyte");
}


constexpr auto InputTC_007 = R"yaml(
name: test
[invalid: yaml
)yaml";

TEST(BtxDescriptor, TC_007_Negative_InvalidYaml) {
    const auto res = lexy::load_descriptor(InputTC_007);
    ASSERT_FALSE(res.has_value());
    // yaml-cpp error message contains "bad file" or similar
    EXPECT_FALSE(res.error().message.empty());
}


constexpr auto InputTC_008 = R"yaml(
id: 1
)yaml";

TEST(BtxDescriptor, TC_008_Negative_MissingName) {
    const auto res = lexy::load_descriptor(InputTC_008);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Missing mandatory field: name");
}


constexpr auto InputTC_009 = R"yaml(
name: test
message:
  name: test_msg
  id: 1
  fields:
    - name: data
      type: string
      length: unknown_field
      length_type: byte
)yaml";

TEST(BtxDescriptor, TC_009_Negative_InvalidReference) {
    // TODO(tech-debt): This depends on decoding capability.
    // TODO(feature): Implement sanity check in `descriptor`.
    const auto desc_res = lexy::load_descriptor(InputTC_009);
    ASSERT_TRUE(desc_res.has_value());

    const std::string bin = "hello";
    const auto parse_res  = lexy::decode(*desc_res, nova::data_view(bin));
    EXPECT_FALSE(parse_res.has_value());
    EXPECT_TRUE(parse_res.error().message.find("Length reference not found") != std::string::npos);
}

constexpr auto InputTC_010 = R"yaml(
name: test
id: 1
)yaml";

TEST(BtxDescriptor, TC_010_Negative_MissingMessage) {
    const auto res = lexy::load_descriptor(InputTC_010);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().message, "Missing mandatory field: message");
}

} // namespace
