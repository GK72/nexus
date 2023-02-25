#pragma once

#include "nxs/utils.h"

#include <fmt/format.h>

#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nxs {

namespace detail {
    struct def_parser {
        [[nodiscard]] std::string operator()(std::ifstream&& inf) {
            std::stringstream ss;
            for (std::string line; std::getline(inf, line); ) {
                ss << line << '\n';
            }
            return ss.str();
        }
    };
}

template <class Callable = identity>
class line_parser {
public:
    line_parser(Callable&& callback = {})
        : m_callback(callback)
    {}

    template <class T = std::remove_cvref_t<std::invoke_result_t<Callable, std::string&>>>
    [[nodiscard]] auto operator()(std::ifstream&& inf) {
        auto ret = std::vector<T>();
        for (std::string line; std::getline(inf, line); ) {
            ret.push_back(m_callback(line));
        }
        return ret;
    }

private:
    Callable m_callback;
};

template <class Parser = detail::def_parser>
[[nodiscard]] auto read_file(std::string_view path, Parser&& parser = {})
        -> std::expected<std::remove_cvref_t<std::invoke_result_t<Parser, std::ifstream>>, error>
{
    const auto fs = std::filesystem::path(path);
    if (not std::filesystem::is_regular_file(fs)) {
        return std::unexpected<error>(fmt::format("{} is not a regular file!", std::filesystem::absolute(fs).string()));
    }

    return parser(std::ifstream(fs));
}

} // namespace nxs
