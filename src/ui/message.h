/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 *     Message
 */

#pragma once

#include <string>

#include "action.h"
#include "elements.h"
#include "types.h"

namespace nxs {

class UI;

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


} // namespace nxs
