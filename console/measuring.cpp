#include <deque>
#include <iostream>
#include <list>
#include <string_view>
#include <vector>

#include "utility.h"

using glib::getMeasureStats;
using glib::measure;
using glib::gint;

struct Data256 {
    char value[256];
};

template <class T> void runMeasures(int sizeinit, int sizemax, int inc, int nRuns, const std::string_view& type) {
    for (size_t size = sizeinit; size <= sizemax; size += inc) {
        std::vector<T> vec{1};
        std::list<T>   list{1};
        std::deque<T>  deque{1};

        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { vec.push_back({ 1 }); }}, nRuns), "Vector - push_back", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { list.push_back({ 1 }); }}, nRuns), "List - push_back", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { deque.push_back({ 1 }); }}, nRuns), "Deque - push_back", size, true);

        auto tmp = vec.front().value;
        std::cout << getMeasureStats(measure([&]() {
            for (const auto& e : vec) { tmp += e.value[0]; }}, nRuns), "Vector - read", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (const auto& e : list) { tmp += e.value[0]; }}, nRuns), "List - read", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (const auto& e : deque) { tmp += e.value[0]; }}, nRuns), "Deque - read", size, true);

        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { vec.erase(vec.begin()); }}, nRuns), "Vector - push_back", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { list.pop_front(); }}, nRuns), "List - push_back", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { deque.pop_front(); }}, nRuns), "Deque - push_back", size, true);
    }
}


int main(int argc, char* args[])
{
    int sizeinit = std::stoi(args[1]);
    int sizemax = std::stoi(args[2]);
    int inc = std::stoi(args[3]);
    int nRuns = std::stoi(args[4]);

    runMeasures<Data256>(sizeinit, sizemax, inc, nRuns, "Data256");

}