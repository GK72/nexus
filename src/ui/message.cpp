/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - implementation
 *     Message
 */

#include "message.h"

#include "ui.h"

namespace nxs {

void UIMessage::execute() {
    m_ui->clearScreen();
    display();
    m_ui->waitKey();
}


} // namespace nxs
