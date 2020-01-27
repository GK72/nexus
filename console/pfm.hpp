// **********************************************
// ** gkpro @ 2020-01-27                       **
// **                                          **
// **         Performance measuring            **
// **                                          **
// **                                          **
// **********************************************

#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "utility.h"

namespace glib::pfm
{

using gint = size_t;

template <class T>
void multiInsert() {
    std::vector<T> vec(100000);
    std::vector<T> vec2(1);
    vec2.insert(vec2.end(), vec.begin(), vec.end());
}

template <class T>
void backInserterCopy() {
    std::vector<T> vec(100000);
    std::vector<T> vec2(1);
    std::copy(vec.begin(), vec.end(), std::back_inserter(vec2));
}

template <class T>
void backInserterMove() {
    std::vector<T> vec(10000);
    std::vector<T> vec2(1);
    std::move(vec.begin(), vec.end(), std::back_inserter(vec2));
}


template <class Time = std::chrono::microseconds, class Function>
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

template <class Time = std::chrono::microseconds>
struct MeasureStats {
    std::string name;
    Time best { 0 };
    Time worst { 0 };
    Time average { 0 };
    Time median { 0 };
    gint noOfMeasures { 0 };

    std::string toString() const {
        std::string res;
        if constexpr (std::is_same<Time, std::chrono::nanoseconds>::value) { res = "ns"; }
        else if constexpr (std::is_same<Time, std::chrono::microseconds>::value) { res = "us"; }
        else if constexpr (std::is_same<Time, std::chrono::milliseconds>::value) { res = "ms"; }

        return "Name         : " + name + '\n' +
               "Best time    : " + std::to_string(best.count())    + ' ' + res + '\n' +
               "Worst time   : " + std::to_string(worst.count())   + ' ' + res + '\n' +
               "Average time : " + std::to_string(average.count()) + ' ' + res + '\n' +
               "Median time  : " + std::to_string(median.count())  + ' ' + res + '\n' +
               "# measures   : " + std::to_string(noOfMeasures);
    }

    std::string toCSV() const {
        return glib::joinStr(","
            ,name
            ,std::to_string(best.count())
            ,std::to_string(worst.count())
            ,std::to_string(average.count())
            ,std::to_string(median.count())
            ,std::to_string(noOfMeasures));
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