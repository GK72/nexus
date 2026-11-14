/**
 * Part of Nexus Library
 *
 * @brief   Heimdall's configuration: which repositories it tracks.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-14
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/config.hpp"

#include "heimdall/style.hpp"

#include <algorithm>
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

auto dashboard_sections() -> const std::vector<std::string>& {
    static const std::vector<std::string> sections { "system", "repos", "tasks", "memory" };
    return sections;
}

auto parse_config(const nova::dyn_value& doc) -> nova::expected<config_t, nova::error> {
    config_t config;
    try {
        config.roots = doc.lookup<std::vector<std::string>>("roots", config.roots);
        config.exclude_from_roots = doc.lookup<std::vector<std::string>>("exclude-from-roots", {});
        config.repositories = doc.lookup<std::vector<std::string>>("repositories", {});
        config.norn = doc.lookup<std::vector<std::string>>("norn", config.norn);
        config.projects = doc.lookup<std::map<std::string, std::string>>("projects", {});
        config.claude = doc.lookup<std::string>("claude", config.claude);
        config.memory = doc.lookup<std::vector<std::string>>("memory", {});
        config.dashboard = doc.lookup<std::vector<std::string>>("dashboard.sections", config.dashboard);
        if (config.norn.empty()) {
            return nova::wrap_error(std::errc::invalid_argument, "norn: the command is empty");
        }
        for (const auto& section : config.dashboard) {
            if (not std::ranges::contains(dashboard_sections(), section)) {
                return nova::wrap_error(std::errc::invalid_argument,
                                        "dashboard.sections: unknown section '" + section
                                            + "' (system, repos, tasks, memory)");
            }
        }
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
