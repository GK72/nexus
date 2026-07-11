#pragma once

#include <optional>
#include <string>

namespace baldr::log {

/**
 * @brief   Logging output mode for the default `nova::log` logger.
 */
enum class mode {
    standard, ///< Plain, appendable spdlog lines (unchanged `nova::log` behavior).
    progress  ///< In-place, overwritable progress lines rendered via `baldr::progress`.
};

/**
 * @brief   Resolve the effective logging mode.
 *
 * Precedence: `cli_override` > `BALDR_LOG_MODE` environment variable >
 * `isatty(stderr)` auto-detection > `mode::standard` default.
 *
 * @param   cli_override    Explicit mode requested via the CLI, if any.
 *
 * @return  The resolved mode.
 */
auto resolve_mode(std::optional<mode> cli_override = std::nullopt) -> mode;

/**
 * @brief   Initialize the default spdlog logger with the sink matching `m`.
 *
 * In `mode::standard`, behavior is byte-identical to `nova::log::init(name)`.
 * In `mode::progress`, the default logger is backed by a `progress_sink` that
 * renders in-place, overwritable lines via `baldr::progress`, dumping the
 * full buffered history only on `error`/`critical` records.
 *
 * @param   m       The logging mode to initialize with.
 * @param   name    Logger name (also used as the topic name).
 */
void init(mode m, const std::string& name = "baldr");

} // namespace baldr::log
