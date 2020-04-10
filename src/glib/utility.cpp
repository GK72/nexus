// **********************************************
// ** gkpro @ 2019-08-29                       **
// ** G-Library                                **
// **   Utility library implementation         **
// **                                          **
// **********************************************

#include <cstdint>
#include <ctime>
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
    std::cerr << "[LOG] " << std::to_string(time) << " - " << msg;
    //std::cerr << "[LOG] " << glib::joinStr(" - ", std::to_string(time), msg);

}


std::string ipv6Formatter(std::string ipv6)
{
    size_t p = 0;
    size_t q = 0;
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

Random* Random::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new Random();
    }
    return m_instance;
}

int Random::randomInt(int min, int max)
{
    std::uniform_int_distribution<> dist(min, max);
    return dist(m_mt);
}


void Publisher::attach(Subscriber* sub)
{
    _subs.push_back(sub);
}

void Publisher::detach(Subscriber* sub)
{
    _subs.erase(std::find(_subs.begin(), _subs.end(), sub));
}

void Publisher::notify(Event&& evt)
{
    for (auto& e : _subs) e->trigger(evt);
}

size_t Index::at(const std::vector<size_t>& vec) const
{
    m_global = vec[vec.size() - 1];
    size_t dm = 1;
    for (size_t i = vec.size(); i > 1; --i) {
        dm *= m_dims[i-1];
        m_global = vec[i-2] * dm + m_global;
    }
    return m_global;
}

void ThreadPool::joinAll()
{
    for (auto& t : m_threads) {
        t.join();
    }
}

void ThreadPool::message(const std::string& msg, int id) {
        std::scoped_lock lock(m_mx);
        glib::print("# ", id, msg);
    }



} //End of namespace glib