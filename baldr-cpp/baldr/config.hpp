#pragma once

#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace baldr {

/**
 * @brief Configuration settings for Baldr.
 */
struct Config {
    std::string build_type = "Debug";
    std::string target = "all";
    int jobs = 1;
    std::vector<std::string> cmake_args;
    
    // Compiler settings
    std::optional<std::string> cc;
    std::optional<std::string> cxx;

    /**
     * @brief Load configuration from hierarchical locations.
     * 
     * Lookup order (later overrides earlier):
     * 1. ~/.config/baldr/config.yaml
     * 2. ~/.baldr.yaml
     * 3. ./.baldr.yaml (relative to project_path)
     * 
     * @param project_path Optional path to the project directory.
     * @returns A Config object with merged settings.
     */
    static auto load(std::optional<std::filesystem::path> project_path = std::nullopt) -> Config;
    
    /**
     * @brief Merge settings from a YAML node into this configuration.
     * 
     * @param node The YAML node containing settings.
     */
    void merge(const YAML::Node& node);
};

} // namespace baldr
