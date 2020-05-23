// **********************************************
// ** gkpro @ 2020-04-25                       **
// **                                          **
// **           ---  G-Library  ---            **
// **          Utility library header          **
// **                                          **
// **********************************************

#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace glib {


#define _g_NIE(msg) throw NotImplementedException(msg)
#define _g_uNIE throw NotImplementedException("Unknown Exception")

#ifndef NDEBUG
#define DBGMSG(msg, value) glib::print("", "[DBG] ", __FILE__, ":", __FUNCTION__, ":", __LINE__,  "  \"", msg, "\"  = ", value);
#else
#define DBGMSG(msg, value) ((void)0)
#endif

using pDim = std::vector<size_t>;                   // For passing dimensions as parameter

// ---------------------------------------==[ Concepts ]==--------------------------------------- //

template <class T>
concept Range = requires(T& t) {
    std::begin(t);
    std::end(t);
};


// ------------------------------------==[ Free Functions ]==------------------------------------ //

void dumpError(const std::exception& ex, const std::string_view& sv = "");
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

// TODO: clean up header

// ********************************************************************************************** //
//                                             Classes                                            //
// ********************************************************************************************** //

class Random {
public:
    static Random* getInstance();
    static int randomInt(int min, int max);

protected:
    Random();

private:
    static Random* m_instance;
    static std::random_device m_rd;
    static std::mt19937 m_mt;
};


struct Event {
    Event(const std::string& str) : msg(str) {}
    std::string msg;
};

class Subscriber;

class Publisher {
public:
    virtual void attach(Subscriber* sub);
    virtual void detach(Subscriber* sub);
    virtual void notify(Event&& evt);

protected:
    Publisher() {}
    std::vector<Subscriber*> _subs;

private:
};

class Subscriber {
public:
    virtual ~Subscriber() {}                      // in subclass: { _sub->detach(this); }
    virtual void trigger(Event& evt) = 0;

protected:
    Subscriber() {}
};



class ThreadPool {
public:
    template <class F> void add(F func) {
        m_threads.emplace_back(func);
    }
    void joinAll();
    void message(const std::string& msg, int id);

private:
    std::vector<std::thread> m_threads;
    std::mutex m_mx;
};

// ********************************************************************************************** //
//                                         Access functions                                       //
// ********************************************************************************************** //

template <class T> struct Iterator {
    Iterator()                              { p = nullptr; }
    Iterator(T* p)                          : p(p) {}
    bool operator!=(Iterator rhs)           { return p != rhs.p; }
    T& operator*()                          { return *p; }
    Iterator& operator++()                  { ++p; return *this; }
    Iterator operator++(int) {
        Iterator<T> t(p);
        ++(*p);
        return t;
    }

    T* p;
};

class Index {
public:
    Index(const std::vector<size_t>& dims) : m_dims(dims) {}
    size_t at(const std::vector<size_t>& vec) const;
    size_t at(size_t x, size_t y) const { return x * m_dims[1] + y; }

private:
    std::vector<size_t> m_dims;
    mutable size_t m_global = 0;
    size_t m_tile = 0;
    size_t m_local = 0;
};

// ********************************************************************************************** //
//                                           Exceptions                                           //
// ********************************************************************************************** //

class NotImplementedException : public std::runtime_error {
public:
    NotImplementedException(const char* msg) : std::runtime_error(msg) {}
};

class IOErrorException : public std::runtime_error {
public:
    IOErrorException(const char* filename) : std::runtime_error("IO Error") {}
};



} // namespace glib


// compile-time endianness swap based on http://stackoverflow.com/a/36937049 
//template<class T, std::size_t... N>
//constexpr T bswap_impl(T i, std::index_sequence<N...>) {
//  return (((i >> N*CHAR_BIT & std::uint8_t(-1)) << (sizeof(T)-1-N)*CHAR_BIT) | ...);
//}
//static_assert(bswap<std::uint16_t>(0x1234u)==0x3412u);
//
//template<typename ...Args>
//void printer(Args&&... args) {
//    (std::cout << ... << args) << '\n';
//}
