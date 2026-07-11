#pragma once

#include <cstddef>
#include <deque>
#include <string>

#include <fmt/format.h>

#include <baldr/tty.hpp>
#include <baldr/tui_debug.hpp>

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
