/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 */

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "types.h"

namespace nxs {

class Menu;
class UIInputGrid;

class UI {
public:
    using InputLambda = std::function<void(const std::string&, int)>;

    UI();
    ~UI();

    int         input(Menu& menu);
    int         input(UIInputGrid& menu);
    std::string input(CoordsRC pos, const std::string& defaultStr = "");
    std::string input(CoordsRC pos, InputLambda process, const std::string& defaultStr = "");

    void refresh();
    void render(CoordsRC coords, std::string_view text, const Format& format = {});
    void clearScreen();
    void waitKey();

private:
    int m_previewSelection = 0;
    std::string m_previewSelected;

    int getChar();

    void highlightOn();
    void highlightOff();
};


} // namespace nxs
