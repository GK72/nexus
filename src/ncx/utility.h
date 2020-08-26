/*
 * gkpro @ 2020-04-25
 *   Nexus Library
 *   Utility header
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
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

#ifdef __cpp_concepts
template <class T>
concept Range = requires(T& t) {
    std::begin(t);
    std::end(t);
};
#endif

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
         [](const bool x)                        { return x ? "true" : "false"; }
        ,[](const char x)                        { return std::to_string(x); }
        ,[](const short x)                       { return std::to_string(x); }
        ,[](const int x)                         { return std::to_string(x); }
        ,[](const long x)                        { return std::to_string(x); }
        ,[](const unsigned char x)               { return std::to_string(x); }
        ,[](const unsigned short x)              { return std::to_string(x); }
        ,[](const unsigned int x)                { return std::to_string(x); }
        ,[](const size_t x)                      { return std::to_string(x); }
        ,[](const float x)                       { return std::to_string(x); }
        ,[](const char* x)                       { return std::string(x); }
        ,[](char* x)                             { return std::string(x); }
        ,[](std::string_view x)                  { return std::string(x); }
        ,[](const std::string& x)                { return x; }
        ,[](const std::chrono::seconds& x)       { return std::to_string(x.count()) + " s"; }
        ,[](const std::chrono::milliseconds& x)  { return std::to_string(x.count()) + " ms"; }
        ,[](const std::chrono::microseconds& x)  { return std::to_string(x.count()) + " us"; }
        ,[](const std::chrono::nanoseconds& x)   { return std::to_string(x.count()) + " ns"; }
        ,[](const auto& x)                       { return x.toString(); }
    };
    return stringify(std::forward<T>(in));
}

template <class T, class ...Ts>
[[nodiscard]] std::string joinStr(const std::string& separator, T&& first, Ts&&... args) {
    return toString(std::forward<T>(first)) +
        (... + (separator + toString(std::forward<Ts>(args))));
}

template <class T, class ...Ts>
void print(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << toString(std::forward<T>(first));
    (std::cout << ... << (separator + toString(std::forward<Ts>(args)))) << '\n';
}

template <class T, class ...Ts>
void printr(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << toString(std::forward<T>(first));
    (std::cout << ... << (separator + toString(std::forward<Ts>(args)))) << '\r' << std::flush;
}

template <class T>
void print(T&& t) {
    std::cout << toString(std::forward<T>(t)) << '\n';
}

template <class T>
void printr(T&& t) {
    std::cout << toString(std::forward<T>(t)) << '\r' << std::flush;
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

template <class Time = std::chrono::nanoseconds> Time now() {
    return std::chrono::duration_cast<Time>(
        std::chrono::system_clock().now().time_since_epoch());
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
template
#ifdef __cpp_concepts
    <Range KeysCont
    ,Range ValueCont
#else
    <class KeysCont
    ,class ValueCont
#endif
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

// -------------------------------------==[ Generators ]==--------------------------------------- //

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

// --------------------------------------==[ Classes ]==----------------------------------------- //

class Output {
public:
    Output(std::string_view path)
        : m_out(path.data())
    {}

    ~Output() = default;

    Output(const Output&)           = delete;
    Output(Output&&)                = delete;
    Output operator=(const Output&) = delete;
    Output operator=(Output&&)      = delete;

    template <class T, class... Ts>
    void write(const std::string& separator, T&& first, Ts&&... args) {
        m_out << joinStr(
            separator,
            std::forward<T>(first),
            std::forward<Ts>(args)...
        ) << '\n';
    }

private:
    std::ofstream m_out;
};

class Progress {
public:
    Progress(size_t total);

    void update(size_t progress);
    void prefix(const std::string& prefix)  { m_prefix = prefix; }
    void suffix(const std::string& suffix)  { m_suffix = suffix; }
    void fill(char ch)                      { m_fill = ch; }

private:
    size_t m_total;
    size_t m_size = 50;
    char   m_fill = '=';
    // const char* m_fill = "\u2588";

    std::string m_prefix;
    std::string m_suffix;
    std::chrono::seconds m_start { 0 };
};


} // namespace nxs
