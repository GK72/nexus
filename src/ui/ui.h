#pragma once

#include <string>
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
    virtual ~UI() = default;

    virtual void create()                  = 0;
    virtual void addMenu(const Menu& menu) = 0;

    virtual void waitKey()                                    = 0;
    virtual void input(const std::string& msg)                = 0;
    virtual void clear()                                      = 0;
    virtual void print(const TextBox& text)                   = 0;
    virtual void print(const TextBox& text, int row, int col) = 0;

    virtual void setPrintHighlightOn()  = 0;
    virtual void setPrintHighlightOff() = 0;

    virtual MenuAction* messageNewScreen(const std::string& msg) = 0;
    virtual void updateSize()                                    = 0;
};

class TextUI : public UI {
public:
    TextUI();
    ~TextUI() override;

    void create() override;
    void addMenu(const Menu& menu) override;

    void waitKey() override;
    void input(const std::string& msg) override;

    void clear() override;
    void print(const TextBox& text) override;
    void print(const TextBox& text, int row, int col) override;

    void setPrintHighlightOn() override;
    void setPrintHighlightOff() override;

    MenuAction* messageNewScreen(const std::string& msg) override;
    void updateSize() override;

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

} // namespace nxs
