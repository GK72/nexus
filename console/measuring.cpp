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
            for (auto& e : vec) { ++e.value[0]; }}, nRuns), "Vector - write", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (auto& e : list) { ++e.value[0]; }}, nRuns), "List - write", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (auto& e : deque) { ++e.value[0]; }}, nRuns), "Deque - write", size, true);

        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { vec.erase(vec.begin()); }}, nRuns), "Vector - pop_front", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { list.pop_front(); }}, nRuns), "List - pop_front", size, true);
        std::cout << getMeasureStats(measure([&]() {
            for (gint i = 0; i < size; ++i) { deque.pop_front(); }}, nRuns), "Deque - pop_front", size, true);


        std::cout << getMeasureStats(measure([&]() {
            auto half = vec.begin();
            std::advance(half, vec.size() / 2);
            vec.erase(vec.begin(), half);
        }, nRuns), "Vector - split_half", size, true);

        std::cout << getMeasureStats(measure([&]() {
            auto half = list.begin();
            std::advance(half, list.size() / 2);
            list.erase(list.begin(), half);
        }, nRuns), "List - split_half", size, true);

        std::cout << getMeasureStats(measure([&]() {
            auto half = deque.begin();
            std::advance(half, deque.size() / 2);
            deque.erase(deque.begin(), half);
        }, nRuns), "Deque - split_half", size, true);
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