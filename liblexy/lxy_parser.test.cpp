#include <gtest/gtest.h>
#include <liblexy/descriptor.hpp>

namespace {

constexpr auto LxySample = R"lxy(
message player {
    int message_length : 8;
    int version : 4;
    int id : 12;
    int is_alive : 1;
    int health : 7;
    int name_length : 8;
    str name : @name_length;
}
)lxy";

TEST(LxyParserTest, Positive_ParseSample) {
    const auto res = lexy::load_descriptor_lxy(LxySample);
    ASSERT_TRUE(res.has_value()) << "Failed to parse .lxy descriptor: " << res.error().message;

    const auto& desc = *res;
    EXPECT_EQ(desc.message.name, "player");
    EXPECT_EQ(desc.message.fields.size(), 7);

    EXPECT_EQ(desc.message.fields[0].name, "message_length");
    EXPECT_EQ(desc.message.fields[0].type, lexy::descriptor::field_type::unsigned_integer);
    EXPECT_EQ(std::get<std::size_t>(desc.message.fields[0].length), 8);
    EXPECT_EQ(desc.message.fields[0].len_type, lexy::descriptor::length_type::bit);

    EXPECT_EQ(desc.message.fields[1].name, "version");
    EXPECT_EQ(desc.message.fields[1].type, lexy::descriptor::field_type::unsigned_integer);
    EXPECT_EQ(std::get<std::size_t>(desc.message.fields[1].length), 4);
    EXPECT_EQ(desc.message.fields[1].len_type, lexy::descriptor::length_type::bit);

    EXPECT_EQ(desc.message.fields[2].name, "id");
    EXPECT_EQ(desc.message.fields[2].type, lexy::descriptor::field_type::unsigned_integer);
    EXPECT_EQ(std::get<std::size_t>(desc.message.fields[2].length), 12);
    EXPECT_EQ(desc.message.fields[2].len_type, lexy::descriptor::length_type::bit);

    EXPECT_EQ(desc.message.fields[3].name, "is_alive");
    EXPECT_EQ(desc.message.fields[3].type, lexy::descriptor::field_type::unsigned_integer); // or boolean? yaml has it as bool
    EXPECT_EQ(std::get<std::size_t>(desc.message.fields[3].length), 1);
    EXPECT_EQ(desc.message.fields[3].len_type, lexy::descriptor::length_type::bit);

    EXPECT_EQ(desc.message.fields[4].name, "health");
    EXPECT_EQ(desc.message.fields[4].type, lexy::descriptor::field_type::unsigned_integer);
    EXPECT_EQ(std::get<std::size_t>(desc.message.fields[4].length), 7);
    EXPECT_EQ(desc.message.fields[4].len_type, lexy::descriptor::length_type::bit);

    EXPECT_EQ(desc.message.fields[5].name, "name_length");
    EXPECT_EQ(desc.message.fields[5].type, lexy::descriptor::field_type::unsigned_integer);
    EXPECT_EQ(std::get<std::size_t>(desc.message.fields[5].length), 8);
    EXPECT_EQ(desc.message.fields[5].len_type, lexy::descriptor::length_type::bit);

    EXPECT_EQ(desc.message.fields[6].name, "name");
    EXPECT_EQ(desc.message.fields[6].type, lexy::descriptor::field_type::string);
    EXPECT_EQ(std::get<std::string>(desc.message.fields[6].length), "name_length");
    EXPECT_EQ(desc.message.fields[6].len_type, lexy::descriptor::length_type::byte);
}

} // namespace
