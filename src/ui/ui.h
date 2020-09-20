#pragma once

#include <string>
#include <utility>
#include <vector>

namespace nxs {

class UI;

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
    virtual void execute() = 0;
};

class Message : public MenuAction {
public:
    Message(UI* ui, const std::string msg, int x = 0, int y = 0);
    ~Message() = default;

    void execute() override;

protected:
    std::string m_msg;
    int m_x;
    int m_y;

    UI* m_ui;
};

class MessageNewScreen : public Message {
public:
    MessageNewScreen(UI* ui, const std::string& msg, int x = 0, int y = 0);
    ~MessageNewScreen() = default;

    void execute() override;
};

template <class Func>
class ActionCallback : public MenuAction {
public:
    ActionCallback(Func func) : m_func(func) {}
    ~ActionCallback() = default;
    void execute() override { m_func(); }

private:
    Func m_func;
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
    using InputLambda = void(*)(const std::string&);

    UI();
    ~UI();

    void create();
    void addMenu(const Menu& menu);

    void waitKey();

    std::string input(const std::string& msg, const std::string& defaultStr = "");
    std::string input(const std::string& msg, InputLambda process, const std::string& defaultStr = "");

    void clear();
    void print(const TextBox& text);
    void print(const TextBox& text, int row, int col);

    void highlightOn();
    void highlightOff();

    MenuAction* messageNewScreen(const std::string& msg);
    void updateSize();

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
    std::pair<int, int> getCur();
    void restoreCur();
    void saveCur();
};

} // namespace nxs
