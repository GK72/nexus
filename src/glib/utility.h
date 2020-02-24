// **********************************************
// ** gkpro @ 2020-01-28                       **
// **                                          **
// **           ---  G-Library  ---            **
// **          Utility library header          **
// **                                          **
// **********************************************

#pragma once
#include <algorithm>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

namespace glib {


#define _g_NIE(msg) throw NotImplementedException(msg)
#define _g_uNIE throw NotImplementedException("Unknown Exception")

using gint = size_t;
using pDim = std::vector<gint>;                   // For passing dimensions as parameter


// ********************************************************************************************** //
//                                            Functions                                           //
// ********************************************************************************************** //

void dumpError(const std::exception& ex, const std::string_view& sv = "");
void printLog(const std::string_view& msg);
std::string ipv6Formatter(std::string ipv6);

inline gint subtractClip(gint lhs, gint rhs) {
    return lhs < rhs ? 0 : lhs - rhs;
}
inline std::string padBoth(const std::string& str, gint count, const char ch = ' ');

inline std::string padBegin(const std::string& str, gint count, const char ch = ' ') {
    return std::string(subtractClip(count, str.size()), ch) + str;
}

inline std::string padEnd(const std::string& str, gint count, const char ch = ' ') {
    return str + std::string(subtractClip(count, str.size()), ch);
}

inline void printn() {
    std::cout << '\n'; 
}


// ********************************************************************************************** //
//                                       Templated functions                                      //
// ********************************************************************************************** //

template <class ...Ts> struct Lambdas : Ts... {
    Lambdas(Ts&& ...t) : Ts(std::forward<Ts>(t))... {}
    using Ts::operator()...;
};

template<class ...Ts> Lambdas(Ts...) -> Lambdas<std::decay_t<Ts>...>;

template <class T> std::string toString(T&& in) {
    Lambdas vis{
        [](const int x)                         { return std::to_string(x); },
        [](const gint x)                        { return std::to_string(x); },
        [](const char* x)                       { return std::string(x); },
        [](const std::string& x)                { return x; },
        [](const std::chrono::microseconds& x)  { return std::to_string(x.count()) + " us"; },
        [](const std::chrono::milliseconds& x)  { return std::to_string(x.count()) + " ms"; },
        [](const std::chrono::nanoseconds&  x)  { return std::to_string(x.count()) + " ns"; }
    };
    return vis(std::forward<T>(in));
}


template <class T, class ...Ts>
std::string joinStr(std::string separator, T&& first, Ts&&... args) {
    return toString(first) + (... + (separator + toString(args)));
}

template <class T, class ...Ts>
void print(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << toString(first);
    (std::cout << ... << (separator + toString(args))) << '\n';
}

template <class T>
void printn(T t) {
    std::cout << toString(t) << '\n';
}

template <class T> T swapEndian32(T& x) {
    return (x << 24) & 0xFF000000 |
           (x <<  8) & 0x00FF0000 |
           (x >>  8) & 0x0000FF00 |
           (x >> 24) & 0x000000FF ;
}


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
    Index(const std::vector<gint>& dims) : m_dims(dims) {}
    gint at(const std::vector<gint>& vec) const;
    gint at(gint x, gint y) const { return x * m_dims[1] + y; }

private:
    std::vector<gint> m_dims;
    mutable gint m_global = 0;
    gint m_tile = 0;
    gint m_local = 0;
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



} // End of namespace glib


// compile-time endianness swap based on http://stackoverflow.com/a/36937049 
//template<class T, std::gint... N>
//constexpr T bswap_impl(T i, std::index_sequence<N...>) {
//  return (((i >> N*CHAR_BIT & std::uint8_t(-1)) << (sizeof(T)-1-N)*CHAR_BIT) | ...);
//}
//static_assert(bswap<std::uint16_t>(0x1234u)==0x3412u);
//
//template<typename ...Args>
//void printer(Args&&... args) {
//    (std::cout << ... << args) << '\n';
//}