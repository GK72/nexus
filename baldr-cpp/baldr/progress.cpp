#include <baldr/progress.hpp>

#include <fstream>
#include <string>

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

    // Erasing everything below the cursor (rather than clearing one row per
    // buffered message) is a cheap, defensive redraw-correctness measure,
    // e.g. in case `m_max_lines` shrinks between frames.
    tty::erase_to_end();

    std::size_t max_width = tty::terminal_width() > 0 ? tty::terminal_width() - 1 : 0;
    for (auto& line: m_buffer) {
        // TODO: Indent.
        std::string capped = tty::cap_visible_width(line, max_width);
        fmt::println("{}", capped);
        tui_debug("{}", capped);
    }

    tty::cursor_up(m_buffer.size());
}

void progress::success(const std::string& msg) {
    tty::erase_to_end();

    if (not msg.empty()) {
        // TODO: Timestamps, spdlog.
        fmt::println("\033[1;34m{}", msg);
    }
}

void progress::failure(const std::string& msg) {
    tty::erase_to_end();

    for (auto& line: m_buffer) {
        // TODO: Indent.
        fmt::println("\033[0;31m{}", line);
    }

    if (not msg.empty()) {
        // TODO: Timestamps, spdlog.
        fmt::println("\033[1;31m{}", msg);
    }
}

} // namespace baldr
