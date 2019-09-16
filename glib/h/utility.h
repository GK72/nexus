// **********************************************
// ** gkpro @ 2019-09-04                       **
// **                                          **
// **           ---  G-Library  ---            **
// **          Utility library header          **
// **                                          **
// **********************************************

#pragma once
#include <algorithm>
#include <chrono>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

namespace glib {

using pDim = std::vector<size_t>;                   // For passing dimensions as parameter

template <class ...Ts> void print(Ts&&... args) {
    (std::cout << ... << args) << '\n';
}

template <class T> T swapEndian32(T& x) {
    x = (x << 24) & 0xFF000000 |
        (x <<  8) & 0x00FF0000 |
        (x >>  8) & 0x0000FF00 |
        (x >> 24) & 0x000000FF ;
    return x;
}

// ************************************************************************** //
//                           Performance measuring                            //
// ************************************************************************** //

template <class T> struct vectorloop_postfixInc {
    void operator()(T& c) {
        size_t size = c.size();
        for (size_t i = 0; i != size; i++) {
            c[i] = ~c[i];
        }
    }
};

template <class T> struct vectorloop_prefixInc {
    void operator()(T& c) {
        size_t size = c.size();
        for (size_t i = 0; i != size; ++i) {
            c[i] = ~c[i];
        }
    }
};

template <class T> struct vectorloop_iterator {
    void operator()(T& c) {
        size_t size = c.size();
        for (auto it = c.begin(); it != c.end(); ++it) {
            *it = ~*it;
        }
    }
};

template <class T> struct vectorautoloop {
    void operator()(T& c) {
        size_t size = c.size();
        for (auto &e : c) {
            e = ~e;
        }
    }
};

template <class T> struct vectorforeach {
    void operator()(T& c) {
        size_t size = c.size();
        std::for_each(c.begin(), c.end(), [](auto& e){e = ~e;});
    }
};


// Performance measuring in a given container
// Function can be a functor (function object), a named function or a lambda
template <class Time = std::chrono::microseconds
    ,class Container
    ,class Function>
    std::vector<Time> measure(Function func, Container &cont, size_t runcount = 1) {
    std::vector<Time> timeresults;
    for (size_t i = 0; i < runcount; ++i) {
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
template <class Time = std::chrono::microseconds
    ,class Function>
    std::vector<Time> measure(Function func, size_t runcount = 1) {
    std::vector<Time> timeresults;
    for (size_t i = 0; i < runcount; ++i) {
        auto t1 = std::chrono::high_resolution_clock::now();
        func();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto td = std::chrono::duration_cast<Time>(t2 - t1);
        timeresults.push_back(td);
    }
    return timeresults;
}

template <class Time = std::chrono::microseconds>
std::string getMeasureStats(std::vector<Time> &&vec, std::string &&label) {
    std::string str;
    size_t size = vec.size();
    Time min = vec[0];
    Time max = vec[0];
    Time avg = Time{ 0 };

    for (const auto& elem : vec) {
        if (elem < min) min = elem;
        if (elem > max) max = elem;
        avg += elem;
    }
    avg /= size;

    str = "Name         : " + label + '\n' + 
        "Best time    : " + std::to_string(min.count()) + '\n' +
        "Worst time   : " + std::to_string(max.count()) + '\n' +
        "Average time : " + std::to_string(avg.count()) + '\n' +
        "# measures   : " + std::to_string(size) + "\n\n";

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
    index(const std::vector<size_t>& dims);
    size_t at(const std::vector<size_t>& vec) const;
    size_t at(size_t x, size_t y) const;

private:
    std::vector<size_t> dims;
    mutable size_t _global = 0;
    size_t _tile = 0;
    size_t _local = 0;
};

// ************************************************************************** //
//                                 Exceptions                                 //
// ************************************************************************** //

class IOErrorException : public std::exception {
public:
    IOErrorException(const char* filename) : std::exception("IO Error") {}

};



} // End of namespace glib


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
