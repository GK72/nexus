/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     NCurses C++ wrapper
 */

#pragma once

#include <ncurses.h>

#include "utility.h"

namespace nxs::ncg {

template <class T, class ...Ts>
void print(const std::string& separator, T&& first, Ts&&... args) {
    printw(nxs::joinStr(separator, first, (..., args), "\n").c_str());
}

template <class T>
void print(T t) {
    printw((toString(t) + "\n").c_str());
}

template <class T, class ...Ts>
void print(int row, int col, const std::string& separator, T&& first, Ts&&... args) {
    move(row, col);
    print(separator, first, (..., args));
}

template <class T>
void print(int row, int col, T t) {
    move(row, col);
    printw((toString(t)).c_str());
}


} // namespace nxs::ncg
