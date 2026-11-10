/**
 * Part of Nexus Library
 *
 * @brief   Heimdall's configuration: which repositories it tracks.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/config.hpp"

#include "heimdall/style.hpp"

#include <cstdlib>
#include <exception>
#include <system_error>

namespace heimdall {

auto config_path() -> std::filesystem::path {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");  // NOLINT(concurrency-mt-unsafe): read-only
    const auto base = xdg != nullptr and *xdg != '\0'
        ? std::filesystem::path(xdg)
        : std::filesystem::path(expand_home("~/.config"));
    return base / "nexus" / "heimdall.toml";
}

auto parse_config(const nova::dyn_value& doc) -> nova::expected<config_t, nova::error> {
    config_t config;
    try {
        config.roots = doc.lookup<std::vector<std::string>>("roots", config.roots);
        config.exclude_from_roots = doc.lookup<std::vector<std::string>>("exclude-from-roots", {});
        config.repositories = doc.lookup<std::vector<std::string>>("repositories", {});
    } catch (const std::exception& ex) {
        return nova::wrap_error(std::errc::invalid_argument, ex.what());
    }
    return config;
}

auto load_config(const std::filesystem::path& path) -> nova::expected<config_t, nova::error> {
    std::error_code ec;
    if (not std::filesystem::exists(path, ec)) {
        return config_t{};
    }
    auto doc = nova::dyn_value::read_config(path);
    if (not doc) {
        return nova::unexpected<nova::error>{ doc.error() };
    }
    return parse_config(*doc);
}

} // namespace heimdall
