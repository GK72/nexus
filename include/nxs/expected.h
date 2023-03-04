#pragma once

#include <utility>
#include <variant>
#include <string_view>

namespace nxs {

struct error {
    std::string_view msg;

    [[nodiscard]] constexpr
        operator std::string_view() const noexcept {
            return msg;
    }
};

namespace detail {

template <typename E = error>
struct unexpected {
    E value;
};

template <typename T, typename E>
class expected {
public:
    using value_type      = T;
    using error_type      = E;
    using unexpected_type = unexpected<E>;

    template <typename U> constexpr expected(const U& value)
        : _value_or_unexpected(value)
    {}

    constexpr expected(const unexpected_type& unexpected)
        : _value_or_unexpected(unexpected.value)
    {}

    constexpr expected(unexpected_type&& unexpected)
        : _value_or_unexpected(std::move(unexpected.value))
    {}

    [[nodiscard]] constexpr const T* operator->() const noexcept    { return &value(); }
    [[nodiscard]] constexpr T* operator->() noexcept                { return &value(); }
    [[nodiscard]] constexpr const T& operator*() const& noexcept    { return value(); }
    [[nodiscard]] constexpr T& operator*() & noexcept               { return value(); }
    [[nodiscard]] constexpr const T&& operator*() const&& noexcept  { return value(); }
    [[nodiscard]] constexpr T&& operator*() && noexcept             { return value(); }

    [[nodiscard]] constexpr bool has_value() const noexcept         { return std::holds_alternative<T>(_value_or_unexpected); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] constexpr const T& value() const&                 { return std::get<T>(_value_or_unexpected); }
    [[nodiscard]] constexpr T& value() &                            { return std::get<T>(_value_or_unexpected); }
    [[nodiscard]] constexpr const T&& value() const&&               { return std::get<T>(_value_or_unexpected); }
    [[nodiscard]] constexpr T&& value() &&                          { return std::get<T>(_value_or_unexpected); }

    [[nodiscard]] constexpr const E& error() const&                 { return std::get<E>(_value_or_unexpected); }
    [[nodiscard]] constexpr E& error() &                            { return std::get<E>(_value_or_unexpected); }
    [[nodiscard]] constexpr const E&& error() const&&               { return std::get<E>(_value_or_unexpected); }
    [[nodiscard]] constexpr E&& error() &&                          { return std::get<E>(_value_or_unexpected); }

    template <typename U>
    [[nodiscard]] constexpr
    T value_or(U&& def) const& {
        return has_value() ? value() : T{ std::forward(def) };
    }

    template <typename U>
    [[nodiscard]] constexpr
    T value_or(U&& def) && {
        return has_value() ? value() : T{ std::forward(def) };
    }

private:
    std::variant<T, E> _value_or_unexpected;
};

} // namespace detail

template <typename T>
using expected = detail::expected<T, error>;

using unexpected = detail::unexpected<>;

} // namespace nxs
