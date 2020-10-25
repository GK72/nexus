/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 *     Menu
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "elements.h"
#include "types.h"

namespace nxs {

class UI;

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
