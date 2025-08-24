#include <baldr/progress.hpp>

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

namespace baldr {

#ifdef BALDR_TUI_DEBUG
// TODO: Configurable path.
std::ofstream g_tui_debug { "/tmp/baldr_tui.dbg" };

namespace detail {
    void tui_debug_impl(const std::string& msg) {
        g_tui_debug << msg << '\n';
    }
} // namespace detail
#endif

void progress::msg(const std::string& msg) {
    if (m_buffer.size() == m_max_lines) {
        m_buffer.pop_front();
    }

    m_buffer.push_back(msg);

    for (auto& line: m_buffer) {
        ansi::clear_line();
        // TODO: Indent.
        fmt::println("{}", line);
        tui_debug("{}", line);
    }

    ansi::cursor_up(m_buffer.size());
}

void progress::success(const std::string& msg) {
    for (std::size_t i = 0; i < m_buffer.size(); ++i) {
        ansi::clear_line();
        ansi::cursor_down();
    }

    ansi::cursor_up(std::min(m_max_lines, m_buffer.size()));

    if (not msg.empty()) {
        // TODO: Timestamps, spdlog.
        fmt::println("\033[1;34m{}", msg);
    }
}

void progress::failure(const std::string& msg) {
    for (auto& line: m_buffer) {
        ansi::clear_line();
        // TODO: Indent.
        fmt::println("\033[0;31m{}", line);
    }

    if (not msg.empty()) {
        // TODO: Timestamps, spdlog.
        fmt::println("\033[1;31m{}", msg);
    }
}

} // namespace baldr
