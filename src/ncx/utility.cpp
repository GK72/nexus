/*
 * gkpro @ 2020-04-25
 *   Nexus Library
 *   Utility implementation
 */

#include <cstdint>
#include <ctime>
#include "utility.h"

namespace nxs {

void printLog(const std::string_view& msg) {
    auto timestamp = std::chrono::system_clock::now();
    time_t time = std::chrono::system_clock::to_time_t(timestamp);
    std::cerr << "[LOG] " << std::to_string(time) << " - " << msg;
}

std::string ipv6Formatter(const std::string& ipv6) {
    size_t p = 0;
    size_t q = 0;
    std::vector<std::string> segments;
    std::string out;

    // Extracting segments
    while ((q = ipv6.find(':', p)) != std::string::npos) {
        segments.push_back(ipv6.substr(p, q - p));
        p = q + 1;
    }
    segments.push_back(ipv6.substr(p));

    // Removing leading zeroes
    bool isPrevEmpty = false;
    for (auto& s : segments) {
        while (s.size() > 0 && s.at(0) == '0') {
            s.erase(0, 1);
        }
        out += s;
        if (s.size() > 0 || !isPrevEmpty) {
            out += ':';
        }
        isPrevEmpty = s.size() == 0;
    }
    // Removing trailing colon
    out.erase(out.size() - 1, 1);

    return out;
}

Progress::Progress(size_t total)
    : m_total(total)
{
    m_start = now<std::chrono::seconds>();
}

void Progress::update(size_t progress) {
    double ratio  = progress / static_cast<double>(m_total);
    size_t n      = static_cast<size_t>(ratio * m_size);
    size_t movCur = m_size - n;
    std::string progressStr(n, m_fill);

    auto elapsed = now<std::chrono::seconds>() - m_start;

    if (progress == m_total) {
        print("", m_prefix, " |", progressStr, "| ", elapsed, m_suffix);
    }
    else {
        printr("", m_prefix, " |", progressStr, "\033[", movCur, "C", "| ", elapsed, m_suffix);
    }
}


} // namespace nxs
