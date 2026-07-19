#include <baldr/config.hpp>

#include <libnova/log.hpp>
#include <libnova/utils.hpp>
#include <libnova/yaml.hpp>

#include <string>
#include <filesystem>

namespace baldr {

[[nodiscard]] auto parse(const nova::yaml& doc, config result) -> config {
    if (doc.contains("debugger")) {
        result.debugger = doc.lookup<std::string>("debugger");
    }

    if (doc.contains("debugger-args")) {
        result.debugger_args = doc.lookup<std::vector<std::string>>("debugger-args");
    }

    if (doc.contains("build-type")) {
        result.build_type = doc.lookup<std::string>("build-type");
    }

    if (doc.contains("cmake.definitions")) {
        auto defines = doc.lookup<std::map<std::string, std::string>>("cmake.definitions");
        for (auto& [key, value] : defines) {
            result.cmake_defines.insert_or_assign(key, std::move(value));
        }
    }

    return result;
}

auto load(std::string_view yaml_content, config result) -> nova::expected<config, nova::error> {
    try {
        auto doc = nova::yaml{ std::string(yaml_content) };
        return parse(doc, std::move(result));
    } catch (const std::exception& ex) {
        return nova::unexpected{ nova::error(ex.what()) };
    }
}

auto load(const std::filesystem::path& path, baldr::config result) -> nova::expected<baldr::config, nova::error> {
    nova::log::trace("Trying to load config `{}`", path.string());
    if (not std::filesystem::exists(path)) {
        return result;
    }

    nova::log::trace("Config `{}` loaded", path.string());

    try {
        auto doc = nova::yaml{ path };
        return parse(doc, std::move(result));
    } catch (const std::exception& ex) {
        return nova::unexpected{ nova::error(ex.what()) };
    }
}

auto load(const std::filesystem::path& project_dir) -> nova::expected<config, nova::error> {
    const auto home = std::filesystem::path{ nova::getenv("HOME").value() };
    auto config_default = config{ };

    auto global = load(home / ".baldr.yaml", config_default);
    if (global) {
        config_default = *global;
    }

    return load(project_dir / ".baldr.yaml", config_default);
}

} // namespace baldr
