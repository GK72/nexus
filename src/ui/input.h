/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 *     Input
 */

#pragma once

#include <string>
#include <vector>

#include "types.h"
#include "action.h"
#include "elements.h"

namespace nxs {

class UI;

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


} // namespace nxs
