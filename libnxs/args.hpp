#include <libnova/error.hpp>

#include <meta>
#include <span>
#include <sstream>
#include <string_view>

namespace nxs::args {

template <typename Opts>
auto parse(std::span<const std::string_view> args) -> Opts {
    Opts opts;

    constexpr auto mctx = std::meta::access_context::current();
    template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^Opts, mctx))) {
        auto it = std::ranges::find_if(
            args,
            [](std::string_view arg) {
                return arg.starts_with("--")
                    && arg.substr(2) == std::meta::identifier_of(mem);
            }
        );

        if (it == std::end(args)) {
            continue;
        }

        if (it + 1 == std::end(args)) {
            throw nova::exception("Option {} is missing a value", *it);
        }

        using T = [: std::meta::type_of(mem) :];
        auto ss = std::stringstream(it[1]);
        if (ss >> opts.[: mem :]; not ss) {
            throw nova::exception("Failed to parse option {} into a {}", *it, std::meta::display_string_of(^^T));
        }
    }

    return opts;
}

} // namespace nxs::args
