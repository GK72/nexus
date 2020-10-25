/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 *     UI Elements
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types.h"

namespace nxs {

class UI;

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


} // namespace nxs
