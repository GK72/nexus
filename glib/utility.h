// **********************************************
// ** gkpro @ 2019-09-17                       **
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
#include <vector>


namespace glib {


#define _g_NIE(msg) throw NotImplementedException(msg)
#define _g_uNIE throw NotImplementedException("Unknown Exception")

using gint = size_t;
using pDim = std::vector<gint>;                   // For passing dimensions as parameter

void dumpError(const std::exception& ex, const std::string_view& sv = "");
void printLog(const std::string_view& msg);
std::string ipv6Formatter(std::string ipv6);

void print(const std::string_view& sv);

template <class ...Ts> void printm(const std::string& separator, Ts&&... args) {
    (std::cout << ... << (separator + args)) << '\n';
}


template <class T> T swapEndian32(T& x) {
    return (x << 24) & 0xFF000000 |
           (x <<  8) & 0x00FF0000 |
           (x >>  8) & 0x0000FF00 |
           (x >> 24) & 0x000000FF ;
}


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

// ************************************************************************** //
//                           Performance measuring                            //
// ************************************************************************** //

template <class T> struct vectorloop_postfixInc {
    void operator()(T& c) {
        gint size = c.size();
        for (gint i = 0; i != size; i++) {
            c[i] = ~c[i];
        }
    }
};

template <class T> struct vectorloop_prefixInc {
    void operator()(T& c) {
        gint size = c.size();
        for (gint i = 0; i != size; ++i) {
            c[i] = ~c[i];
        }
    }
};

template <class T> struct vectorloop_iterator {
    void operator()(T& c) {
        gint size = c.size();
        for (auto it = c.begin(); it != c.end(); ++it) {
            *it = ~*it;
        }
    }
};

template <class T> struct vectorautoloop {
    void operator()(T& c) {
        gint size = c.size();
        for (auto &e : c) {
            e = ~e;
        }
    }
};

template <class T> struct vectorforeach {
    void operator()(T& c) {
        gint size = c.size();
        std::for_each(c.begin(), c.end(), [](auto& e){e = ~e;});
    }
};


// Performance measuring in a given container
// Function can be a functor (function object), a named function or a lambda
template <class Time = std::chrono::nanoseconds
    ,class Container
    ,class Function>
std::vector<Time> measureCont(Function func, Container &cont, gint runcount = 1) {
    std::vector<Time> timeresults;
    for (gint i = 0; i < runcount; ++i) {
        auto t1 = std::chrono::high_resolution_clock::now();
        func(cont);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto td = std::chrono::duration_cast<Time>(t2 - t1);
        timeresults.push_back(td);
    }
    return timeresults;
}

// Performance measuring
// Function can be a functor (function object), a named function or a lambda
template <class Time = std::chrono::nanoseconds
    ,class Function>
std::vector<Time> measure(Function func, gint runcount = 1) {
    std::vector<Time> timeresults;
    for (gint i = 0; i < runcount; ++i) {
        auto t1 = std::chrono::high_resolution_clock::now();
        func();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto td = std::chrono::duration_cast<Time>(t2 - t1);
        timeresults.push_back(td);
    }
    return timeresults;
}

template <class Time = std::chrono::nanoseconds>
std::string getMeasureStats(std::vector<Time>&& vec, std::string&& label
                          , gint containerSize, bool csv = false) {
    std::string str;
    gint size = vec.size();
    Time min = vec[0];
    Time max = vec[0];
    Time avg = Time{ 0 };

    for (const auto& elem : vec) {
        if (elem < min) min = elem;
        if (elem > max) max = elem;
        avg += elem;
    }
    avg /= size;

    if (!csv) {
        str = "Name         : " + label + '\n' +
            "Best time    : " + std::to_string(min.count()) + '\n' +
            "Worst time   : " + std::to_string(max.count()) + '\n' +
            "Average time : " + std::to_string(avg.count()) + '\n' +
            "Size         : " + std::to_string(containerSize) + '\n' +
            "# measures   : " + std::to_string(size) + "\n\n";
    }
    else {
        str = label + ',' +
            std::to_string(min.count()) + ',' +
            std::to_string(max.count()) + ',' +
            std::to_string(avg.count()) + ',' +
            std::to_string(containerSize) + ',' +
            std::to_string(size) + '\n';
    }

    return str;
}

//  ***  (F as function, C as container)
//  ***  Function pointer
//  ***  template <class C> void function(void (*func)(C &c), T &cont)  { func(cont); }
//  ***  Usage: function(function, c) 
//  ***  
//  ***  Function object with class instantiation
//  ***  template <class F, class C> void functor(C &c)                 { F a; a(c); }
//  ***  Usage: function<functor<C>>(c)
//  ***  
//  ***  Function object 
//  ***  template <class F, class C> void functor(F f, C& c)            { f(c); }
//  ***  Usage: function(functor<C>(), c)


// ************************************************************************** //
//                               Access functions                             //
// ************************************************************************** //

template <class T> struct iterator
{
    iterator();
    iterator(T* p);
    bool operator!=(iterator rhs);
    T& operator*();

    iterator& operator++();
    iterator operator++(int);


    T* p;
};

struct index
{
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

// ************************************************************************** //
//                                 Exceptions                                 //
// ************************************************************************** //

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
