/**
 * Part of Nexus Library
 *
 * @brief   Tests of reading the configuration.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-14
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/config.hpp"

#include <gtest/gtest.h>

namespace {

auto parse(std::string_view toml) -> nova::expected<heimdall::config_t, nova::error> {
    auto doc = nova::dyn_value::parse(toml, nova::dyn_value::format_e::toml);
    EXPECT_TRUE(doc.has_value());
    return heimdall::parse_config(*doc);
}

} // namespace

TEST(Config, Defaults) {
    const auto config = parse("");
    ASSERT_TRUE(config.has_value());
    EXPECT_TRUE(config->roots.empty());
    EXPECT_TRUE(config->exclude_from_roots.empty());
    EXPECT_TRUE(config->repositories.empty());
    EXPECT_EQ(config->norn, std::vector<std::string>{ "norn.py" });
    EXPECT_TRUE(config->projects.empty());
}

TEST(Config, AllKeys) {
    const auto config = parse(
        "roots = [\"~/repos\", \"~/work\"]\n"
        "exclude-from-roots = [\"zsh-*\"]\n"
        "repositories = [\"~/src/one\"]\n"
        "norn = [\"norn.py\", \"--url\", \"http://host:8080\"]\n"
        "[projects]\n"
        "baldr = \"baldr-cpp\"\n");
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->roots.size(), 2);
    EXPECT_EQ(config->exclude_from_roots, std::vector<std::string>{ "zsh-*" });
    EXPECT_EQ(config->repositories, std::vector<std::string>{ "~/src/one" });
    EXPECT_EQ(config->norn.size(), 3);
    EXPECT_EQ(config->projects.at("baldr"), "baldr-cpp");
}

TEST(Config, EmptyNornIsAnError) {
    EXPECT_FALSE(parse("norn = []\n").has_value());
}

TEST(Config, WrongTypeIsAnError) {
    EXPECT_FALSE(parse("roots = \"~/repos\"\n").has_value());
}

TEST(Config, MissingFileMeansDefaults) {
    const auto config = heimdall::load_config("/nonexistent/heimdall.toml");
    ASSERT_TRUE(config.has_value());
    EXPECT_TRUE(config->roots.empty());
}

TEST(Config, DashboardSectionsAndClaude) {
    const auto config = parse("claude = \"/c\"\nmemory = [\"~/m\"]\n[dashboard]\nsections = [\"tasks\", \"system\"]\n");
    ASSERT_TRUE(config.has_value()) << config.error().detail;
    EXPECT_EQ(config->claude, "/c");
    EXPECT_EQ(config->memory, (std::vector<std::string>{ "~/m" }));
    EXPECT_EQ(config->dashboard, (std::vector<std::string>{ "tasks", "system" }));
    EXPECT_EQ(parse("")->dashboard, (std::vector<std::string>{ "system", "repos", "tasks", "memory" }));
}

TEST(Config, UnknownDashboardSectionIsAnError) {
    const auto config = parse("[dashboard]\nsections = [\"weather\"]\n");
    ASSERT_FALSE(config.has_value());
    EXPECT_NE(config.error().detail.find("weather"), std::string::npos);
}
