// **********************************************
// ** gkpro @ 2019-08-29                       **
// ** G-Library                                **
// **   Utility library implementation         **
// **                                          **
// **********************************************

#include <cstdint>
#include "utility.h"
#include "utility_t.hpp"
#include "gmath.h"

namespace glib {


Random* Random::m_instance = nullptr;
std::random_device Random::m_rd;
std::mt19937 Random::m_mt;


void dumpError(const std::exception& ex, const std::string_view& sv)
{
    if (sv.empty()) {
        printLog(ex.what());
}
    else {
        std::string prefix = "\n***** Dump ****\n";
        std::string suffix = "\n***************\n";
        std::string data = sv.data();
        printLog(ex.what() + prefix + data + suffix);
    }
}

void printLog(const std::string_view& msg)
{
    auto timestamp = std::chrono::system_clock::now();
    time_t time = std::chrono::system_clock::to_time_t(timestamp);
    std::cerr << "[LOG] " << std::put_time(std::localtime(&time), "%T") << " - " << msg;
}


std::string ipv6Formatter(std::string ipv6) {
    gint p = 0;
    gint q = 0;
    std::vector<std::string> segments;
    std::string out;

    // Extracting segments
    while ((q = ipv6.find(':', p)) != std::string::npos) {
        segments.push_back(ipv6.substr(p, q - p));
        p = q + 1;
    }
    segments.push_back(ipv6.substr(p));

    // Removing leading zeroes
    bool isPrevEmpty = false;
    for (auto& s : segments) {
        while (s.size() > 0 && s.at(0) == '0') {
            s.erase(0, 1);
        }
        out += s;
        if (s.size() > 0 || !isPrevEmpty) {
            out += ':';
        }
        isPrevEmpty = s.size() == 0;
    }
    // Removing trailing colon
    out.erase(out.size() - 1, 1);

    return out;
}


Random::Random()
{
    m_mt = std::mt19937(m_rd());
}

Random* Random::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new Random();
    }
    return m_instance;
}
int Random::randomInt(int min, int max) {
    std::uniform_int_distribution<> dist(min, max);
    return dist(m_mt);
}


void Publisher::attach(Subscriber* sub) {
    _subs.push_back(sub);
}

void Publisher::detach(Subscriber* sub) {
    _subs.erase(std::find(_subs.begin(), _subs.end(), sub));
}

void Publisher::notify(Event&& evt) {
    for (auto& e : _subs) e->trigger(evt);
}

template <class T> iterator<T>::iterator()                          { p = nullptr; }
template <class T> iterator<T>::iterator(T* p)                      : p(p) {}
template <class T> bool iterator<T>::operator!=(iterator rhs)       { return p != rhs.p; }
template <class T> T& iterator<T>::operator*()                      { return *p; }
template <class T> iterator<T>& iterator<T>::operator++()           { ++p; return *this; }
template <class T> iterator<T> iterator<T>::operator++(int)
{
    iterator<T> t(p);
    ++(*p);
    return t;
}

index::index(const std::vector<gint>& dim) : dims(dim) {}

gint index::at(const std::vector<gint>& vec) const
{
    _global = vec[vec.size() - 1];
    gint dm = 1;
    for (gint i = vec.size(); i > 1; --i) {
        dm *= dims[i-1];
        _global = vec[i-2] * dm + _global;
    }
    return _global;
}

gint index::at(gint x, gint y) const { return x * dims[1] + y; }



} //End of namespace glib