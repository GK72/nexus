/*
 * gkpro @ 2020-09-27
 *   Nexus Library
 *     Text based User Interface - implementation
 */

#include "ui.h"

#include <functional>
#include <string>

#include "ncg.h"

namespace nxs {

UI::UI() {
    setlocale(LC_ALL, "");
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
}

UI::~UI() {
    nocbreak();
    echo();
    keypad(stdscr, FALSE);
    endwin();
}

void UI::highlightOn() {
    attron(A_REVERSE);
}

void UI::highlightOff() {
    attroff(A_REVERSE);
}

void UI::clearScreen() {
    clear();
}

int UI::getChar() {
    return getch();
}

void UI::waitKey() {
    getch();
}

void UI::render(CoordsRC coords, std::string_view text, const Format& format) {
    if (format.highlighted) {
        highlightOn();
    }

    ncg::print(
        coords.r,
        coords.c,
        text.data()
    );

    highlightOff();
}

std::string UI::input(CoordsRC pos, InputLambda process, const std::string& defaultStr) {
    std::string inputStr = defaultStr;
    m_previewSelected = "";

    render(pos, inputStr);

    while (true) {
        int cursorMov = 0;
        switch (int ch = getch()) {
        case 10:    // KEY: Enter
            return !m_previewSelected.empty() ? m_previewSelected : inputStr;
        case 127:
        case KEY_BACKSPACE:
            if (!inputStr.empty()) {
                move(pos.r, pos.c - 1);
                clrtoeol();
                inputStr.pop_back();
                render(pos, inputStr);
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

        render(pos, inputStr);
        process(inputStr, cursorMov);
    }
}

std::string UI::input(CoordsRC pos, const std::string& defaultStr) {
    return input(pos, [](const std::string& str, int) {}, defaultStr);
}

int UI::input(Menu& menu) {
    menu.display();
    bool loop = true;
    while (loop) {
        switch (getChar()) {
            case 'j':   menu.incCur();   break;
            case 'k':   menu.decCur();   break;
            case 'o':   menu.select();   break;
            case 'q':   loop = false;    break;
            default :                    break;
        }
        clear();
        menu.display();
    }

    return menu.cursor();
}

// TODO: DRY
int UI::input(UIInputGrid& menu) {
    menu.display();
    bool loop = true;
    while (loop) {
        switch (getChar()) {
            case 'j':   menu.incCur();   break;
            case 'k':   menu.decCur();   break;
            case 'o':   menu.select();   break;
            case 'q':   loop = false;    break;
            default :                    break;
        }
        clear();
        menu.display();
    }

    return menu.cursor();
}

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

void UIMessage::execute() {
    m_ui->clearScreen();
    display();
    m_ui->waitKey();
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
    refresh();
}

UIInput::UIInput(
    UI* ui,
    CoordsRC coords,
    const std::string& name,
    const Format& format,
    const std::string& defaultInput
)
    : UIContent(ui, coords, name, format)
    , m_inputStr(defaultInput)
{}

void UIInput::display(CoordsRC offset) {
    m_inputOffset = CoordsRC{ 0, m_pos.c + static_cast<int>(m_name.size()) + 1 };
    m_offset = offset;

    m_ui->render(m_pos + m_offset, m_name, m_format);
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

// void UI::preview(const std::vector<std::string>& elems, int cursorMov) {
    // if (elems.empty()) { return; }

    // if (cursorMov == 1) {
        // m_previewSelection = std::max(0, m_previewSelection - 1);
    // }
    // if (cursorMov == 2) {
        // m_previewSelection = std::min(
            // static_cast<int>(elems.size()) - 1,
            // m_previewSelection + 1
        // );
    // }

    // printw("\n");
    // clrtoeol();
    // m_previewSelected = elems[m_previewSelection];
    // ncg::print(m_previewSelected);
// }

} // namespace nxs
