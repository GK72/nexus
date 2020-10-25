/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - implementation
 */

#include "ui.h"

#include "action.h"
#include "elements.h"
#include "input.h"
#include "menu.h"
#include "message.h"

#include "ncg.h"

namespace nxs {

UI::UI() {
    setlocale(LC_ALL, "");
    initscr();
    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
}

UI::~UI() {
    nocbreak();
    echo();
    keypad(stdscr, FALSE);
    curs_set(1);
    endwin();
}

void UI::highlightOn() {
    attron(A_REVERSE);
}

void UI::highlightOff() {
    attroff(A_REVERSE);
}

void UI::clearScreen() {
    clear();
}

int UI::getChar() {
    return getch();
}

void UI::waitKey() {
    getch();
}

void UI::refresh() {
    ::refresh();
}

void UI::render(CoordsRC coords, std::string_view text, const Format& format) {
    if (format.highlighted) {
        highlightOn();
    }

    ncg::print(
        coords.r,
        coords.c,
        text.data()
    );

    highlightOff();
}

std::string UI::input(CoordsRC pos, InputLambda process, const std::string& defaultStr) {
    std::string inputStr = defaultStr;
    m_previewSelected = "";

    render(pos, inputStr + FULL_BLOCK);

    while (true) {
        int cursorMov = 0;
        switch (int ch = getch()) {
        case 10:    // KEY: Enter
            return !m_previewSelected.empty() ? m_previewSelected : inputStr;
        case 127:
        case KEY_BACKSPACE:
            if (!inputStr.empty()) {
                move(pos.r, pos.c - 1);
                clrtoeol();
                inputStr.pop_back();
                render(pos, inputStr + FULL_BLOCK);
            }
            break;
        case KEY_UP:    cursorMov = 1;  break;
        case KEY_DOWN:  cursorMov = 2;  break;
        case KEY_LEFT:  cursorMov = 3;  break;
        case KEY_RIGHT: cursorMov = 4;  break;
        default:
            inputStr.push_back(ch);
            break;
        }

        render(pos, inputStr + FULL_BLOCK);
        process(inputStr, cursorMov);
    }
}

std::string UI::input(CoordsRC pos, const std::string& defaultStr) {
    return input(pos, [](const std::string& str, int) {}, defaultStr);
}

int UI::input(Menu& menu) {
    menu.display();
    bool loop = true;
    while (loop) {
        switch (getChar()) {
            case 'j':   menu.incCur();   break;
            case 'k':   menu.decCur();   break;
            case 'o':   menu.select();   break;
            case 'q':   loop = false;    break;
            default :                    break;
        }
        clear();
        menu.display();
    }

    return menu.cursor();
}

// TODO: DRY
int UI::input(UIInputGrid& menu) {
    menu.display();
    bool loop = true;
    while (loop) {
        switch (getChar()) {
            case 'j':   menu.incCur();   break;
            case 'k':   menu.decCur();   break;
            case 'o':   menu.select();   break;
            case 'q':   loop = false;    break;
            default :                    break;
        }
        clear();
        menu.display();
    }

    return menu.cursor();
}

// void UI::preview(const std::vector<std::string>& elems, int cursorMov) {
    // if (elems.empty()) { return; }

    // if (cursorMov == 1) {
        // m_previewSelection = std::max(0, m_previewSelection - 1);
    // }
    // if (cursorMov == 2) {
        // m_previewSelection = std::min(
            // static_cast<int>(elems.size()) - 1,
            // m_previewSelection + 1
        // );
    // }

    // printw("\n");
    // clrtoeol();
    // m_previewSelected = elems[m_previewSelection];
    // ncg::print(m_previewSelected);
// }


} // namespace nxs
