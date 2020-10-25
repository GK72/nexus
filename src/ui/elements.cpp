/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - implementation
 *     UI Elements
 */

#include "elements.h"

#include "ui.h"
#include "utility.h"

namespace nxs {

UIFrame::UIFrame(UI* ui, CoordsRC coords, CoordsRC length, UIElement* element)
    : UIElement(ui, coords)
    , m_element(element)
{
    m_length = length;
}

void UIFrame::display(CoordsRC offset) {
    const auto line = nxs::repeat(BOX_HLINE, m_length.c);

    m_ui->render(CoordsRC{ m_pos.r                 , m_pos.c }, line);
    m_ui->render(CoordsRC{ m_pos.r + m_length.r - 1, m_pos.c }, line);

    for (int i = 1; i < m_length.r - 1; ++i) {
        m_ui->render(CoordsRC{ m_pos.r + i, m_pos.c }, BOX_VLINE);
        m_ui->render(CoordsRC{ m_pos.r + i, m_pos.c + m_length.c - 1 }, BOX_VLINE);
    }

    m_ui->render(CoordsRC{ m_pos.r                 , m_pos.c                  }, BOX_TOP_LEFT);
    m_ui->render(CoordsRC{ m_pos.r                 , m_pos.c + m_length.c - 1 }, BOX_TOP_RIGHT);
    m_ui->render(CoordsRC{ m_pos.r + m_length.r - 1, m_pos.c                  }, BOX_BTM_LEFT);
    m_ui->render(CoordsRC{ m_pos.r + m_length.r - 1, m_pos.c + m_length.c - 1 }, BOX_BTM_RIGHT);

    if (m_element) {
        m_element->display(CoordsRC{ m_pos.r + 1, m_pos.c + 1 });
    }
}

void UIContainer::display(CoordsRC offset) {
    // ui->render(this, m_pos);     // TODO: e.g. border, background
    for (const auto& element : m_elements) {
        element->display(m_pos + offset);
    }
}

void UIContainer::add(std::unique_ptr<UIElement> element) {
    m_elements.push_back(std::move(element));
}

UIContent::UIContent(UI* ui, CoordsRC coords, const std::string& name, const Format& format)
    : UIElement(ui, coords)
    , m_name(name)
    , m_format(format)
{}

void UIContent::display(CoordsRC offset) {
    m_ui->render(m_pos + offset, m_name, m_format);
    m_ui->refresh();
}


} // namespace nxs
