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
#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <string_view>
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

gint subtractClip(gint lhs, gint rhs);
std::string padBoth(const std::string& str, gint count, const char ch = ' ');
std::string padEnd(const std::string& str, gint count, const char ch = ' ');
std::string padBegin(const std::string& str, gint count, const char ch = ' ');

void printn();
void printn(const std::string_view& sv);

// ********************************************************************************************** //
//                                       Templated functions                                      //
// ********************************************************************************************** //

template <class ...Ts> struct Visitor : Ts... {
    template <class ...Ts> Visitor(Ts&& ...t) : Ts(std::forward<Ts>(t))... {}
    using Ts::operator()...;
};

template<class ...Ts> Visitor(Ts...) -> Visitor<std::decay_t<Ts>...>;

template <class T> std::string toString(T&& in) {
    Visitor vis{
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

// ********************************************************************************************** //
//                                         Access functions                                       //
// ********************************************************************************************** //

template <class T> struct iterator {
    iterator();
    iterator(T* p);
    bool operator!=(iterator rhs);
    T& operator*();
    iterator& operator++();
    iterator operator++(int);

    T* p;
};

struct index {
public:
    index(const std::vector<gint>& dims);
    gint at(const std::vector<gint>& vec) const;
    gint at(gint x, gint y) const;

private:
    std::vector<gint> dims;
    mutable gint _global = 0;
    gint _tile = 0;
    gint _local = 0;
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
