#pragma once

#include <string>
#include <vector>

namespace glib::ui {

// -----------------------------------==[ Formatted Text ]==----------------------------------------

struct TextFormat {
    bool highlighted = false;
};

struct FormattedText {
    std::string content;
    TextFormat format;
};

using TextBox = std::vector<FormattedText>;

// ----------------------------------------==[ Menu ]==---------------------------------------------

class MenuAction {
public:
    virtual ~MenuAction() = default;
    virtual int execute() = 0;
};

class UIMsg : public MenuAction {
public:
    UIMsg(const std::string msg) : m_msg(msg) {}
    ~UIMsg() = default;
    int execute() override;

private:
    std::string m_msg;
};


struct MenuItem {
    std::string name;
    MenuAction* action;
};

class Menu {
public:
    Menu() = default;
    Menu(const std::vector<MenuItem>& elems) : m_elems(elems) {}

    void decCur();
    void incCur();
    void select() const;

    auto getElems()  const         { return m_elems; }
    auto getCursor() const         { return m_cursor; }

    TextBox toFormattedText() const;

private:
    std::vector<MenuItem> m_elems;
    int m_cursor = 0;
};

// -----------------------------------------==[ UI ]==----------------------------------------------

class UI {
public:
    UI();
    ~UI();

    void input(const std::string& msg);
    void run();
    void addMenu(const Menu& menu);

    void updateSize();

    void print(const TextBox& text);
    void print(const TextBox& text, int row, int col);

    void setPrintHighlightOn();
    void setPrintHighlightOff();

private:
    int m_maxRows;
    int m_maxCols;
    int m_tCurX;
    int m_tCurY;

    int m_lastch;
    bool m_terminate = false;

    Menu m_menu;

    int getChar();

    void initMenu(Menu& menu);
    void displayMenu(const Menu& menu);
    void restoreCur();
    void saveCur();
};

} // namespace glib::ui
