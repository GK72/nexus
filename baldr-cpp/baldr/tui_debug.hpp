#pragma once

#include <string>

#include <fmt/format.h>

namespace baldr {

#ifdef BALDR_TUI_DEBUG
namespace detail {
    void tui_debug_impl(const std::string& msg);
} // namespace detail

/**
 * @brief   Append a formatted debug message to the TUI debug log, when
 *          `BALDR_TUI_DEBUG` is enabled.
 */
template <typename ...Args>
inline void tui_debug(fmt::format_string<Args...> fmt, Args &&...args) {
    detail::tui_debug_impl(fmt::format(fmt, std::forward<Args>(args)...));
}
#else
/**
 * @brief   No-op stand-in for `tui_debug()` when `BALDR_TUI_DEBUG` is
 *          disabled.
 */
template <typename ...Args>
inline void tui_debug([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args &&...args) { }
#endif

} // namespace baldr
