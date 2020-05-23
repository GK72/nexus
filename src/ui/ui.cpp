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

namespace nxs {

Message::Message(UI* ui, const std::string msg, int x, int y)
    : m_ui(ui), m_msg(msg), m_x(x), m_y(y)
{}

void Message::execute() {
    m_ui->print(TextBox{{ m_msg }}, m_x, m_y);
}

MessageNewScreen::MessageNewScreen(UI* ui, const std::string& msg, int x, int y)
    : Message(ui, msg, x, y)
{}

void MessageNewScreen::execute() {
    m_ui->clear();
    Message::execute();
    m_ui->waitKey();
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


TextUI::TextUI() {
    initscr();
    keypad(stdscr, TRUE);
    noecho();
}

TextUI::~TextUI() {
    endwin();
}

void TextUI::create() {
    initMenu(m_menu);
}

void TextUI::initMenu(Menu& menu) {
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
        clear();
        displayMenu(menu);
    }
}

void TextUI::addMenu(const Menu& menu) {
    m_menu = menu;
}

void TextUI::displayMenu(const Menu& menu) {
    restoreCur();
    print(menu.toFormattedText());
    refresh();
}

void TextUI::saveCur() {
    getyx(stdscr, m_tCurY, m_tCurX);
}

void TextUI::restoreCur() {
    move(m_tCurY, m_tCurX);
}

int TextUI::getChar() {
    m_lastch = getch();
    return m_lastch;
}

void TextUI::clear() {
    ::clear();
    refresh();
}

void TextUI::print(const TextBox& text) {
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

void TextUI::print(const TextBox& text, int row, int col) {
    move(row, col);
    print(text);
}

MenuAction* TextUI::messageNewScreen(const std::string& msg) {
    return new MessageNewScreen(this, msg);
}

void TextUI::setPrintHighlightOn() {
    attron(A_REVERSE);
}

void TextUI::setPrintHighlightOff() {
    attroff(A_REVERSE);
}

void TextUI::waitKey() {
    getch();
}

void TextUI::input(const std::string& msg) {
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

void TextUI::updateSize() {
    getmaxyx(stdscr, m_maxRows, m_maxCols);
}

} // namespace nxs
