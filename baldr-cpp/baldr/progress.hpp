#pragma once

#include <concepts>
#include <deque>
#include <functional>
#include <string>

#include <fmt/format.h>

namespace baldr {

#ifdef BALDR_TUI_DEBUG
namespace detail {
    void tui_debug_impl(const std::string& msg);
} // namespace detail

template <typename ...Args>
inline void tui_debug(fmt::format_string<Args...> fmt, Args &&...args) {
    detail::tui_debug_impl(fmt::format(fmt, std::forward<Args>(args)...));
}
#else
template <typename ...Args>
inline void tui_debug([[maybe_unused]] fmt::format_string<Args...> fmt, [[maybe_unused]] Args &&...args) { }
#endif

} // namespace baldr

namespace ansi {

inline void clear_line() {
    baldr::tui_debug("Clear line");
    fmt::print("\x1b[2K");
}

template <std::integral T = int>
inline void cursor_up(T n = 1) {
    baldr::tui_debug("Cursor up: {}", n);
    fmt::print("\x1b[{}A", n);
}

template <std::integral T = int>
inline void cursor_down(T n = 1) {
    baldr::tui_debug("Cursor down: {}", n);
    fmt::print("\x1b[{}B", n);
}

} // namespace ansi

namespace baldr {

class progress {
public:
    void lines(int lines) {
        m_max_lines = static_cast<std::size_t>(lines);
    }

    void msg(const std::string& msg);
    void success(const std::string& msg = "");
    void failure(const std::string& msg = "");

private:
    std::size_t m_max_lines = 1;
    std::deque<std::string> m_buffer;

};


} // namespace baldr
