/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - implementation
 *     UI Menu
 */

#include "menu.h"

#include "ui.h"

namespace nxs {

Menu::Menu(UI* ui, std::vector<Option>&& options, CoordsRC coords)
    : m_options(std::move(options))
    , UIElement(ui, coords)
{}

void Menu::decCur() {
    if (m_cursor == 0) { m_cursor = m_options.size() - 1; }
    else { --m_cursor; }
}

void Menu::incCur() {
    if (m_cursor == m_options.size() - 1) { m_cursor = 0; }
    else { ++m_cursor; }
}

void Menu::select() const {
    m_options[m_cursor].action->execute();
}

void Menu::display(CoordsRC offset) {
    createUIElement()->display(offset);
}

// TODO: no UIElement contruction, work with existing ones
std::unique_ptr<UIElement> Menu::createUIElement() const {
    auto container = std::make_unique<UIContainer>(m_ui, m_pos);

    size_t i = 0;
    for (const auto& option : m_options) {
        container->add(
            std::make_unique<UIContent>(
                m_ui,
                CoordsRC{static_cast<int>(i), 0},
                option.name,
                Format{ i == m_cursor }
            )
        );

        ++i;
    }

    return container;
}


} // namespace nxs
