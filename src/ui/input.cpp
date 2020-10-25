/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - implementation
 *     Input
 */

#include "input.h"

#include "ui.h"

namespace nxs {

UIInputGrid::UIInputGrid(UI* ui, CoordsRC coords, const std::vector<UIInput>& inputs)
    : UIElement(ui, coords)
    , m_inputs(inputs)
{}

void UIInputGrid::display(CoordsRC offset) {
    for (int i = 0; auto& element : m_inputs) {
        if (i == m_cursor) { element.highlightOn(); }
        else { element.highlightOff(); }
        element.display(CoordsRC{ i, 0 });
        ++i;
    }
}

void UIInputGrid::decCur() {
    if (m_cursor == 0) { m_cursor = m_inputs.size() - 1; }
    else { --m_cursor; }
}

void UIInputGrid::incCur() {
    if (m_cursor == m_inputs.size() - 1) { m_cursor = 0; }
    else { ++m_cursor; }
}

void UIInputGrid::select() {
    m_inputs[m_cursor].execute();
}


UIInput::UIInput(UI* ui,
                 CoordsRC coords,
                 const std::string& name,
                 const Format& format,
                 const std::string& defaultInput)
    : UIContent(ui, coords, name, format)
    , m_inputStr(defaultInput)
{}

void UIInput::display(CoordsRC offset) {
    m_inputOffset = CoordsRC{ 0, m_pos.c + static_cast<int>(m_name.size()) + 1 };
    m_offset = offset;

    m_ui->render(m_pos + m_offset              , m_name, m_format);
    m_ui->render(m_pos + offset + m_inputOffset, m_inputStr);
}

void UIInput::execute() {
    m_inputStr = m_ui->input(m_pos + m_offset + m_inputOffset, m_inputStr);
}

void UIInput::highlightOn() {
    m_format.highlighted = true;
}

void UIInput::highlightOff() {
    m_format.highlighted = false;
}


} // namespace nxs
