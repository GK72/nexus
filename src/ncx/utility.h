/*
 * gkpro @ 2020-04-25
 *   Nexus Library
 *   Utility header
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nxs {

#ifndef NDEBUG
#define DBGMSG(msg, value) nxs::print("", "[DBG] ", __FILE__, ":", __FUNCTION__, ":", __LINE__,  "  \"", msg, "\"  = ", value);
#else
#define DBGMSG(msg, value) ((void)0)
#endif

// ---------------------------------------==[ Concepts ]==--------------------------------------- //

template <class T>
concept Range = requires(T& t) {
    std::begin(t);
    std::end(t);
};

// ------------------------------------==[ Free Functions ]==------------------------------------ //

void printLog(const std::string_view& msg);
std::string ipv6Formatter(std::string ipv6);

[[nodiscard]] inline size_t subtractClip(size_t lhs, size_t rhs) {
    return lhs < rhs ? 0 : lhs - rhs;
}

inline void print() {
    std::cout << '\n';
}

// ---------------------------------==[ Templated Functions ]==---------------------------------- //

template <class ...Ts> struct Lambdas : Ts... {
    Lambdas(Ts&& ...t) : Ts(std::forward<Ts>(t))... {}
    using Ts::operator()...;
};

template<class ...Ts> Lambdas(Ts...) -> Lambdas<std::decay_t<Ts>...>;

// ----== String functions ==----

template <class T>
[[nodiscard]] std::string toString(T&& in) {
    Lambdas stringify{
         [](const char x)                        { return std::to_string(x); }
        ,[](const short x)                       { return std::to_string(x); }
        ,[](const int x)                         { return std::to_string(x); }
        ,[](const long x)                        { return std::to_string(x); }
        ,[](const unsigned char x)               { return std::to_string(x); }
        ,[](const unsigned short x)              { return std::to_string(x); }
        ,[](const unsigned int x)                { return std::to_string(x); }
        ,[](const size_t x)                      { return std::to_string(x); }
        ,[](const char* x)                       { return std::string(x); }
        ,[](std::string_view x)                  { return std::string(x); }
        ,[](const std::string& x)                { return x; }
        ,[](const std::chrono::seconds& x)       { return std::to_string(x.count()) + " s"; }
        ,[](const std::chrono::milliseconds& x)  { return std::to_string(x.count()) + " ms"; }
        ,[](const std::chrono::microseconds& x)  { return std::to_string(x.count()) + " us"; }
        ,[](const std::chrono::nanoseconds& x)   { return std::to_string(x.count()) + " ns"; }
    };
    return stringify(std::forward<T>(in));
}

template <class T, class ...Ts>
[[nodiscard]] std::string joinStr(std::string separator, T&& first, Ts&&... args) {
    return toString(first) + (... + (separator + toString(args)));
}

template <class T, class ...Ts>
void print(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << toString(first);
    (std::cout << ... << (separator + toString(args))) << '\n';
}

template <class T>
void print(T t) {
    std::cout << toString(t) << '\n';
}

template <class T>
[[nodiscard]] std::string padBoth(const T& t, size_t count, const char ch = ' ')
{
    size_t pad = subtractClip(count, toString(t).size()) / 2;
    std::string padL(pad, ch);
    std::string padR(pad + (toString(t).size() % 2 ? 1 : 0), ch);
    return padL + toString(t) + padR;
}

template <class T>
[[nodiscard]] std::string padBegin(const T& t, size_t count, const char ch = ' ')
{
    return std::string(subtractClip(count, toString(t).size()), ch) + toString(t);
}

template <class T>
[[nodiscard]] std::string padEnd(const T& t, size_t count, const char ch = ' ')
{
    return toString(t) + std::string(subtractClip(count, toString(t).size()), ch);
}

// ----== Misc ==----

template <class T> T swapEndian32(T& x) {
    return (x << 24) & 0xFF000000 |
           (x <<  8) & 0x00FF0000 |
           (x >>  8) & 0x0000FF00 |
           (x >> 24) & 0x000000FF ;
}

// -------------------------------------==[ Algorithms ]==--------------------------------------- //

constexpr auto GroupByEqualKeys    = [](const auto& key, const auto& prevKey) { return key == prevKey; };
constexpr auto AggregatePlusEquals = [](auto agg, const auto& value)          { return agg += value; };

/**
 * @brief Groups values by keys given in two containers
 *
 * @param keys      Container for keys
 * @param values    Container for values
 *
 * @param p         Predicate function
 *                      indicating when to do the operation
 *                      key and previous key are passed in
 *
 * @param op        Binary operation called on the aggregate and the current value
 *
 * @return          A container with key-value pairs
 */
template <Range KeysCont
         ,Range ValueCont
         ,class Predicate
         ,class BinOp>
[[nodiscard]]
std::vector<std::pair<typename KeysCont::value_type, typename ValueCont::value_type>>
groupBy(const KeysCont& keys, const ValueCont& values, Predicate p, BinOp op)
{
    std::vector<std::pair<typename KeysCont::value_type, typename ValueCont::value_type>> summary;
    typename ValueCont::value_type aggregate {0};
    auto prevKey = keys.front();

    for (auto [key, value] = std::make_tuple(std::begin(keys), std::begin(values));
            key != std::end(keys) && value != std::end(values);
            std::advance(key, 1), std::advance(value, 1))
    {
        if (p(*key, prevKey)) {
            aggregate = op(aggregate, *value);
        }
        else {
            summary.emplace_back(prevKey, aggregate);
            prevKey = *key;
            aggregate = op(typename ValueCont::value_type{0}, *value);
        }
    }

    summary.emplace_back(prevKey, aggregate);

    return summary;
}

// -------------------------------------==[ Generators ]==------------------------------------------

template <size_t N>
[[nodiscard]] constexpr auto range() {
    std::array<size_t, N> r;
    std::iota(std::begin(r), std::end(r), 0);
    return r;
}

template <size_t N, class Init>
[[nodiscard]] constexpr auto range(Init init) {
    std::array<size_t, N> r;
    std::iota(std::begin(r), std::end(r), init);
    return r;
}

template <class Length>
[[nodiscard]] constexpr auto range(Length len) {
    std::vector<Length> r(len);
    std::iota(std::begin(r), std::end(r), 0);
    return r;
}

template <class Length, class Init>
[[nodiscard]] constexpr auto range(Length len, Init init) {
    std::vector<Length> r(len);
    std::iota(std::begin(r), std::end(r), init);
    return r;
}


} // namespace nxs
