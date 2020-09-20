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


UI::UI() {
    setlocale(LC_ALL, "");
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
}

UI::~UI() {
    endwin();
}

void UI::create() {
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
        clear();
        displayMenu(menu);
    }
}

void UI::addMenu(const Menu& menu) {
    m_menu = menu;
}

void UI::displayMenu(const Menu& menu) {
    move(0, 0);
    print(menu.toFormattedText());
    refresh();
}

std::pair<int, int> UI::getCur() {
    int x, y;
    getyx(stdscr, y, x);
    return { x, y };
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

void UI::clear() {
    ::clear();
    refresh();
}

void UI::print(const TextBox& text) {
    for (const auto& line : text) {
        // TODO: generalize for different formatting options
        if (line.format.highlighted) {
            highlightOn();
        }
        else {
            highlightOff();
        }
        ncg::print(line.content);
    }
}

void UI::print(const TextBox& text, int row, int col) {
    move(row, col);
    print(text);
}

MenuAction* UI::messageNewScreen(const std::string& msg) {
    return new MessageNewScreen(this, msg);
}

void UI::highlightOn() {
    attron(A_REVERSE);
}

void UI::highlightOff() {
    attroff(A_REVERSE);
}

void UI::waitKey() {
    getch();
}

void UI::updateSize() {
    getmaxyx(stdscr, m_maxRows, m_maxCols);
}

std::string UI::input(const std::string& msg, InputLambda process, const std::string& defaultStr) {
    printw(msg.c_str());
    refresh();
    std::string inputStr = defaultStr;
    m_previewSelected = "";

    saveCur();
    printw(inputStr.c_str());

    while (true) {
        int cursorMov = 0;
        switch (int ch = getch()) {
        case 10:    // KEY: Enter
            printw("\n");
            return !m_previewSelected.empty() ? m_previewSelected : inputStr;
        case KEY_BACKSPACE:
            if (!inputStr.empty()) {
                auto [x, y] = getCur();
                move(y, x - 1);
                clrtoeol();
                inputStr.pop_back();
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

        restoreCur();
        printw(inputStr.c_str());
        process(inputStr, cursorMov);
        refresh();
    }
}

std::string UI::input(const std::string& msg, const std::string& defaultStr) {
    return input(msg, [](const std::string& str, int) {}, defaultStr);
}

void UI::preview(const std::vector<std::string>& elems, int cursorMov) {
    if (elems.empty()) { return; }

    if (cursorMov == 1) {
        m_previewSelection = std::max(0, m_previewSelection - 1);
    }
    if (cursorMov == 2) {
        m_previewSelection = std::min(
            static_cast<int>(elems.size()) - 1,
            m_previewSelection + 1
        );
    }

    printw("\n");
    clrtoeol();
    m_previewSelected = elems[m_previewSelection];
    ncg::print(m_previewSelected);
}

} // namespace nxs
