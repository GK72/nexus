// **********************************************
// ** gkpro @ 2020-01-29                       **
// **                                          **
// **         Performance Measuring            **
// **              Header-Only                 **
// **                                          **
// **********************************************

#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "utility.h"

namespace glib::pfm {

using gint = size_t;


template <class T, template <class > class Cont>
class msrContainer
{
public:
    msrContainer(gint count) : m_count(count) {}

    void operator()() {
        for (gint i = 0; i < m_count; ++i) {
            ctn.push_back(T{});
        }
    };

private:
    Cont<T> ctn;
    gint m_count;
};


template <class Time = std::chrono::microseconds, class Function>
std::vector<Time> measure(Function func, gint runcount = 1)
{
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

template <class Time = std::chrono::microseconds>
struct MeasureStats
{
    std::string name;
    Time best { 0 };
    Time worst { 0 };
    Time average { 0 };
    Time median { 0 };
    gint noOfMeasures { 0 };

    std::string toString() const {
        constexpr gint pad = 13;
        return joinStr("\n",
            joinStr(" ", padEnd("Name", pad),           ":", name),
            joinStr(" ", padEnd("Best time", pad),      ":", best),
            joinStr(" ", padEnd("Worst time", pad),     ":", worst),
            joinStr(" ", padEnd("Average time", pad),   ":", average),
            joinStr(" ", padEnd("Median time", pad),    ":", median),
            joinStr(" ", padEnd("# measure", pad),      ":", noOfMeasures)
        );
    }

    std::string toCSV() const {
        return joinStr(",", name, best, worst, average, median, noOfMeasures);
    }
};

template <class Time = std::chrono::microseconds>
MeasureStats<Time> getMeasureStats(std::vector<Time>&& vec, const std::string& label)
{
    MeasureStats<Time> stats;
    stats.name = label;

    stats.noOfMeasures = vec.size();
    if (stats.noOfMeasures == 1) {
        stats.best = vec[0];
        return stats;
    }

    stats.best  = vec[0];
    stats.worst = vec[0];

    for (const auto& elem : vec) {
        if (elem < stats.best) stats.best = elem;
        if (elem > stats.worst) stats.worst = elem;
        stats.average += elem;
    }
    stats.average /= stats.noOfMeasures;

    std::sort(begin(vec), end(vec));
    stats.median = vec.at(stats.noOfMeasures / 2);

    return stats;
}


class pfm {
public:
    pfm(gint runcount) : m_runcount(runcount) {}

    void add(const std::function<void()>& func, const std::string& name) {
        m_functions.push_back({name, func});
    }

    void run() {
        for (const auto& [name, func] : m_functions) {
            m_stats.push_back(getMeasureStats(measure(func, m_runcount), name));
        }
    }

    void print() {
        for (const auto& e : m_stats) {
            glib::printn(e.toString());
            glib::printn();
        }
    }

    void printCSV() {
        glib::print(",", "Name", "Best", "Worst", "Average", "Median", "No of Measures");
        for (const auto& e : m_stats) {
            glib::printn(e.toCSV());
        }
    }

private:
    std::vector<std::pair<std::string, std::function<void()>>> m_functions;
    std::vector<MeasureStats<>> m_stats;
    gint m_runcount;
};


} // End of namespace glib::pfm