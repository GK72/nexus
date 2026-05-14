/**
 * Part of Nexus Library
 *
 * Reflection utilities.
 *
 * @author  Gábor Krisztián Girhiny
 * @date    2026-05-14
 */

#include <libnova/error.hpp>
#include <fmt/format.h>

#include <functional>
#include <meta>
#include <ranges>
#include <string_view>
#include <tuple>
#include <utility>

namespace nxs {
namespace ann {

inline constexpr struct {} fmt_debug{};

} // namespace ann

namespace refl {

/**
 * @brief   Check whether the target has a specific annotation.
 *
 * @param   annotation  The annotation "object", not its type.
 */
consteval auto has_annotation(std::meta::info target, std::meta::info annotation) -> bool {
    for (auto _ : std::meta::annotations_of_with_type(target, std::meta::type_of(annotation))) {
        return true;
    }
    return false;
}

struct formatter {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    template <typename T>
        requires (has_annotation(^^T, ^^ann::fmt_debug))
    auto format(const T& type, auto& ctx) const {
        constexpr auto identifier = std::meta::has_identifier(^^T)
            ? std::meta::identifier_of(^^T)
            : "(unnamed-type)";

        auto out = fmt::format_to(ctx.out(), "{}{{ ", identifier);

        auto delim = [first = true, &out]() mutable {
            if (not first) {
                *out++ = ',';
                *out++ = ' ';
            }

            first = false;
        };

        constexpr auto mctx = std::meta::access_context::current();

        template for (constexpr auto base : std::define_static_array(std::meta::bases_of(^^T, mctx))) {
            delim();

            using U = [: std::meta::type_of(base) :];
            out = fmt::format(out, "{}", static_cast<const U&>(type));
        }

        template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, mctx))) {
            delim();
            std::string_view mem_label = std::meta::has_identifier(mem)
                ? std::meta::identifier_of(mem)
                : "(unnamed-member)";

            out = fmt::format_to(out, ".{}={}", mem_label, type.[: mem :]);
        }

        *out++ = ' ';
        *out++ = '}';

        return out;
    }

};

template <typename E>
    requires (std::meta::is_enumerable_type(^^E))
constexpr auto enum_to_string(E value) -> std::string_view {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == [: e :]) {
            return std::meta::identifier_of(e);
        }
    }

    return "(unnamed)";
}

template <typename E>
    requires (std::meta::is_enumerable_type(^^E))
constexpr auto string_to_enum(std::string_view value) {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == std::meta::identifier_of(e)) {
            return [: e :];
        }
    }

    throw nova::exception("Invalid enumeration: {}", value);
}

namespace detail {

struct function_tracer {
    std::function<void(std::string_view)> logger = [](std::string_view msg) { fmt::println("{}", msg); };

    template <std::meta::info F, typename... Args>
    auto trace(Args&&... args) -> decltype([: F :](std::forward<Args>(args)...)) {
        std::size_t idx = 0;
        fmt::memory_buffer out;

        static constexpr auto zip = std::ranges::zip_view(
            std::define_static_array(std::meta::parameters_of(F)),
            std::define_static_array(std::meta::parameters_of(^^trace<F, Args...>))
        );

        template for (constexpr auto [p, q] : zip) {
            if (idx) {
                fmt::format_to(std::back_inserter(out), " ");
            }

            using U = [: std::meta::type_of(std::meta::variable_of(q)) :];
            fmt::format_to(std::back_inserter(out), "{}={}", std::meta::identifier_of(p), static_cast<U>([: std::meta::variable_of(q) :]));

            ++idx;
        }

        if (idx) {
            fmt::format_to(std::back_inserter(out), " ");
        }

        auto&& ret = std::invoke([: F :], args...);
        fmt::format_to(std::back_inserter(out), "return={}", ret);
        logger(to_string(out));
        return ret;
    }
};

} // namespace detail

[[nodiscard]] inline
auto function_tracer() -> detail::function_tracer& {
    static detail::function_tracer tracer;
    return tracer;
}

} // namespace refl
} // namespace nxs

namespace fmt {

template <typename T>
    requires (nxs::refl::has_annotation(^^T, ^^nxs::ann::fmt_debug))
struct formatter<T> : nxs::refl::formatter {};

} // namespace fmt
