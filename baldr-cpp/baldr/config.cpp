#include <baldr/config.hpp>
#include <fstream>
#include <cstdlib>

namespace baldr {

auto Config::load(std::optional<std::filesystem::path> project_path) -> Config {
    Config config;
    
    std::vector<std::filesystem::path> paths;
    
    // 1. ~/.config/baldr/config.yaml
    const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config_home) {
        paths.push_back(std::filesystem::path(xdg_config_home) / "baldr" / "config.yaml");
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            paths.push_back(std::filesystem::path(home) / ".config" / "baldr" / "config.yaml");
        }
    }
    
    // 2. ~/.baldr.yaml
    const char* home = std::getenv("HOME");
    if (home) {
        paths.push_back(std::filesystem::path(home) / ".baldr.yaml");
    }
    
    // 3. ./.baldr.yaml
    auto p = project_path.value_or(std::filesystem::current_path());
    paths.push_back(p / ".baldr.yaml");
    
    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            try {
                YAML::Node node = YAML::LoadFile(path.string());
                config.merge(node);
            } catch (...) {
                // Ignore parsing errors for now
            }
        }
    }
    
    return config;
}

void Config::merge(const YAML::Node& node) {
    if (node["build_type"]) {
        build_type = node["build_type"].as<std::string>();
    }
    if (node["target"]) {
        target = node["target"].as<std::string>();
    }
    if (node["jobs"]) {
        jobs = node["jobs"].as<int>();
    }
    if (node["cmake_args"] && node["cmake_args"].IsSequence()) {
        for (const auto& arg : node["cmake_args"]) {
            cmake_args.push_back(arg.as<std::string>());
        }
    }
    if (node["cc"]) {
        cc = node["cc"].as<std::string>();
    }
    if (node["cxx"]) {
        cxx = node["cxx"].as<std::string>();
    }
}

} // namespace baldr
