// **********************************************
// ** gkpro @ 2020-05-23                       **
// **                                          **
// **           ---  G-Library  ---            **
// **              Text based UI               **
// **                                          **
// **********************************************

#include "ui.h"

#include <functional>
#include <string>

#include "ncg.h"

namespace glib::ui {

int UIMsg::execute() {
    ncg::print(10, 0, m_msg);
    return 0;
}

void Menu::decCur() {
    if (m_cursor == 0) m_cursor = m_elems.size() - 1;
    else --m_cursor;
}

void Menu::incCur() {
    if (m_cursor == m_elems.size() - 1) m_cursor = 0;
    else ++m_cursor;
}

void Menu::select() const {
    m_elems[m_cursor].action->execute();
}

TextBox Menu::toFormattedText() const {
    TextBox ret;

    size_t cursor = 0;
    for (const auto& menuItem : m_elems) {
        FormattedText text{ menuItem.name, cursor == m_cursor };
        ret.push_back(text);
        ++cursor;
    }

    return ret;
}


UI::UI() {
    initscr();
    keypad(stdscr, TRUE);
    noecho();
}

UI::~UI() {
    endwin();
}

void UI::run() {
    initMenu(m_menu);
}

void UI::initMenu(Menu& menu) {
    saveCur();
    displayMenu(menu);
    while (!m_terminate) {
        switch (getChar()) {
            case 'j':       menu.incCur();              break;
            case 'k':       menu.decCur();              break;
            case 'o':       menu.select();              break;
            case 'q':       m_terminate = true;         break;
            default :                                   break;
        }
        displayMenu(menu);
    }
}

void UI::addMenu(const Menu& menu) {
    m_menu = menu;
}

void UI::displayMenu(const Menu& menu) {
    restoreCur();
    print(menu.toFormattedText());
    refresh();
}

void UI::saveCur() {
    getyx(stdscr, m_tCurY, m_tCurX);
}

void UI::restoreCur() {
    move(m_tCurY, m_tCurX);
}

int UI::getChar() {
    m_lastch = getch();
    return m_lastch;
}

void UI::print(const TextBox& text) {
    for (const auto& line : text) {
        // TODO: generalize for different formatting options
        if (line.format.highlighted) {
            setPrintHighlightOn();
        }
        else {
            setPrintHighlightOff();
        }
        ncg::print(line.content);
    }
}

void UI::print(const TextBox& text, int row, int col) {
    move(row, col);
    print(text);
}

void UI::setPrintHighlightOn() {
    attron(A_REVERSE);
}

void UI::setPrintHighlightOff() {
    attroff(A_REVERSE);
}

void UI::input(const std::string& msg) {
    printw(msg.c_str());
    refresh();
    int ch = getch();

    if (ch == KEY_F(1)) {
        printw("F1 key pressed");
    }
    else {
        printw("Key pressed is: ");
        attron(A_BOLD);
        printw("%c", ch);
        attroff(A_BOLD);
    }

    refresh();
}

void UI::updateSize() {
    getmaxyx(stdscr, m_maxRows, m_maxCols);
}

} // namespace glib::ui
