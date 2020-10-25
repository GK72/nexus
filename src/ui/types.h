/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 *     Types
 */

#pragma once

namespace nxs {

constexpr auto FULL_BLOCK = "\u2588";

constexpr auto BOX_HLINE     = "\u2500";
constexpr auto BOX_HLINE_B   = "\u2501";
constexpr auto BOX_VLINE     = "\u2502";
constexpr auto BOX_VLINE_B   = "\u2503";
constexpr auto BOX_TOP_LEFT  = "\u250c";
constexpr auto BOX_TOP_RIGHT = "\u2510";
constexpr auto BOX_BTM_LEFT  = "\u2514";
constexpr auto BOX_BTM_RIGHT = "\u2518";

struct CoordsRC {
    int r = 0;
    int c = 0;
};

inline CoordsRC operator+(CoordsRC lhs, CoordsRC rhs) {
    return { lhs.r + rhs.r, lhs.c + rhs.c };
}

struct Format {
    bool highlighted = false;
};


} // namespace nxs
