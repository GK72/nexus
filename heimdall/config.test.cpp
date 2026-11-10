/**
 * Part of Nexus Library
 *
 * @brief   Tests of reading the configuration.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
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
    EXPECT_EQ(config->roots, std::vector<std::string>{ "~/repos" });
    EXPECT_TRUE(config->exclude_from_roots.empty());
    EXPECT_TRUE(config->repositories.empty());
}

TEST(Config, AllKeys) {
    const auto config = parse(
        "roots = [\"~/repos\", \"~/work\"]\n"
        "exclude-from-roots = [\"zsh-*\"]\n"
        "repositories = [\"~/src/one\"]\n");
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->roots.size(), 2);
    EXPECT_EQ(config->exclude_from_roots, std::vector<std::string>{ "zsh-*" });
    EXPECT_EQ(config->repositories, std::vector<std::string>{ "~/src/one" });
}

TEST(Config, WrongTypeIsAnError) {
    EXPECT_FALSE(parse("roots = \"~/repos\"\n").has_value());
}

TEST(Config, MissingFileMeansDefaults) {
    const auto config = heimdall::load_config("/nonexistent/heimdall.toml");
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->roots, std::vector<std::string>{ "~/repos" });
}
