#pragma once

#include <array>
#include <type_traits>

namespace nxs {

template <class T>
concept arithmetic = std::is_arithmetic_v<T>;

template <typename T>
struct is_std_array : std::false_type {};

template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

template <class T> struct dependent_false : public std::false_type {};
template <class T> inline constexpr bool dependent_false_v = dependent_false<T>::value;

} // namespace nxs
