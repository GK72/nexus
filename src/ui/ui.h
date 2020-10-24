/*
 * gkpro @ 2020-10-24
 *   Nexus Library
 *     Text based User Interface - header
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nxs {

class UI;

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


/**
 * @brief A UI element which can display itself
 */
class UIElement {
public:
    explicit UIElement(UI* ui, CoordsRC coords) : m_ui(ui), m_pos(coords) {}
    virtual ~UIElement() = default;

    virtual void display(CoordsRC offset) = 0;

    int row() const { return m_pos.r; };
    int col() const { return m_pos.c; };

protected:
    UI* m_ui;
    CoordsRC m_pos;
    CoordsRC m_length;       // TODO: clip at length
};



class UIFrame : UIElement {
public:
    UIFrame(UI* ui, CoordsRC coords, CoordsRC length, UIElement* element);
    ~UIFrame() = default;

    void display(CoordsRC offset = {}) override;
private:
    UIElement* m_element;
};


/**
 * @brief UIElement container
 */
class UIContainer : public UIElement {
public:
    UIContainer(UI* ui, CoordsRC coords) : UIElement(ui, coords) {}
    ~UIContainer() = default;

    void display(CoordsRC offset = {}) override;
    void add(std::unique_ptr<UIElement> element);

private:
    std::vector<std::unique_ptr<UIElement>> m_elements;
};


/**
 * @brief The most fundamental UI building block
 */
class UIContent : public UIElement {
public:
    UIContent(UI* ui, CoordsRC coords, const std::string& name, const Format& format = {});
    ~UIContent() = default;

    void display(CoordsRC offset = {}) override;

protected:
    std::string m_name;
    Format m_format;
};


/**
 * @brief An interface for an action that can be executed
 */
class UIAction {
public:
    virtual ~UIAction() = default;
    virtual void execute() = 0;
};


/**
 * @brief An action that calls a function
 */
template <class Callable>
class UICallback : public UIAction {
public:
    UICallback(Callable func) : m_func(func) {}
    ~UICallback() = default;
    void execute() override { m_func(); }

private:
    Callable m_func;
};


/**
 * @brief A named input field with optional default value
 */
class UIInput : public UIContent, public UIAction {
public:
    UIInput(
        UI* ui,
        CoordsRC coords,
        const std::string& name,
        const Format& format = {},
        const std::string& defaultInput = {}
    );
    ~UIInput() = default;

    void display(CoordsRC offset = {}) override;
    void execute() override;
    void highlightOn();
    void highlightOff();

    const std::string& result() const { return m_inputStr; }

private:
    std::string m_inputStr;
    mutable CoordsRC m_offset;      // TODO: remove this hack
    mutable CoordsRC m_inputOffset; // TODO: remove this hack
};


/**
 * @brief Displays a message on a clear screen
 */
class UIMessage : public UIContent, public UIAction {
public:
    UIMessage(UI* ui, const std::string& msg, CoordsRC coords = {}, const Format& format = {})
        : UIContent(ui, coords, msg, format)
    {}
    ~UIMessage() = default;
    void execute() override;
};


class UIInputGrid : public UIElement {
public:
    UIInputGrid(UI* ui, CoordsRC coords, const std::vector<UIInput>& inputs);
    ~UIInputGrid() = default;

    void display(CoordsRC offset = {}) override;

    void decCur();
    void incCur();
    void select();

    int cursor() const { return m_cursor; }
    std::string result(size_t index) const { return m_inputs[index].result(); }

private:
    std::vector<UIInput> m_inputs;
    int m_cursor = 0;
};


/**
 * @brief A menu that stores named actions
 *
 * Keeps track its own selection and is able to create a
 * `UIElement` formatted based on the selection
 */
class Menu : public UIElement {
public:
    struct Option {
        std::string name;
        std::unique_ptr<UIAction> action;
    };

    Menu(UI* ui, std::vector<Option>&& options, CoordsRC coords = {});

    void display(CoordsRC offset = {}) override;

    void decCur();
    void incCur();
    void select() const;
    int cursor() const { return m_cursor; }

private:
    std::vector<Option> m_options;

    int m_cursor = 0;
    std::unique_ptr<UIElement> createUIElement() const;
};


// -----------------------------------------==[ UI ]==----------------------------------------------

class UI {
public:
    using InputLambda = std::function<void(const std::string&, int)>;

    UI();
    ~UI();

    int         input(Menu& menu);
    int         input(UIInputGrid& menu);
    std::string input(CoordsRC pos, const std::string& defaultStr = "");
    std::string input(CoordsRC pos, InputLambda process, const std::string& defaultStr = "");

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


class MenuBuilder {
public:
    MenuBuilder(UI& ui) : m_ui(&ui) {}

    template <class Callable>
    void add(const std::string& name, Callable callback) {
        m_options.push_back({ name, std::make_unique<UICallback<Callable>>(callback) });
    }

    template <class Action, class ...Args>
    void add(const std::string& name, Args&&... args) {
        m_options.push_back({ name, std::make_unique<Action>(m_ui, args...) });
    }

    Menu build() { return nxs::Menu(m_ui, std::move(m_options)); }

private:
    UI* m_ui;
    std::vector<Menu::Option> m_options;
};


} // namespace nxs
