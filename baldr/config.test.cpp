#include <gtest/gtest.h>

#include <baldr/config.hpp>

#include <string_view>

using namespace std::string_view_literals;

namespace {

constexpr auto global_config = R"yaml(
debugger: gdb
debugger-args:
build-type: Release

env:
  GLOBAL_ENV: global
  CUSTOM_ENV: something

cmake:
  definitions:
    GLOBAL_DEFINE: global
    CUSTOM_DEFINE: something
)yaml"sv;

constexpr auto local_config = R"yaml(

env:
  CUSTOM_ENV: something else
  LOCAL_ENV: local

cmake:
  definitions:
    CUSTOM_DEFINE: something else
    LOCAL_DEFINE: local
)yaml"sv;

TEST(config, Negative_MalformedYamlIsError) {
    const auto res = baldr::load("debugger: [invalid\n"sv, baldr::config{});
    ASSERT_FALSE(res.has_value());
    EXPECT_FALSE(res.error().message.empty());
}

TEST(config, MissingFileReturnsDefaults) {
    const baldr::config res;
    EXPECT_EQ(res.debugger, "gdb");
    EXPECT_EQ(res.build_type, "Debug");
    EXPECT_TRUE(res.cmake_defines.empty());
    EXPECT_TRUE(res.env.empty());
}

TEST(config, ProjectLocalFileIsLoaded) {
    const auto res = baldr::load(local_config, baldr::config{});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->debugger, "gdb");
    EXPECT_EQ(res->build_type, "Debug");
    EXPECT_EQ(res->cmake_defines.at("CUSTOM_DEFINE"), "something else");
    EXPECT_EQ(res->cmake_defines.at("LOCAL_DEFINE"), "local");
    EXPECT_EQ(res->env.at("CUSTOM_ENV"), "something else");
    EXPECT_EQ(res->env.at("LOCAL_ENV"), "local");
}

TEST(config, BuildTypeOverride) {
    const auto res = baldr::load("build-type: Release\n"sv, baldr::config{});
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->build_type, "Release");
}

TEST(config, ConfigMerging) {
    const auto global = baldr::load(global_config, baldr::config{});
    ASSERT_TRUE(global.has_value());

    const auto res = baldr::load(local_config, *global);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->debugger, "gdb");
    EXPECT_EQ(res->build_type, "Release");
    EXPECT_EQ(res->cmake_defines.at("GLOBAL_DEFINE"), "global");
    EXPECT_EQ(res->cmake_defines.at("CUSTOM_DEFINE"), "something else");
    EXPECT_EQ(res->cmake_defines.at("LOCAL_DEFINE"), "local");
    EXPECT_EQ(res->env.at("GLOBAL_ENV"), "global");
    EXPECT_EQ(res->env.at("CUSTOM_ENV"), "something else");
    EXPECT_EQ(res->env.at("LOCAL_ENV"), "local");
}

} // namespace
