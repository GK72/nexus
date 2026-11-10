/**
 * Part of Nexus Library
 *
 * @brief   Heimdall: the state of every tracked Git repository at a glance.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#include "heimdall/heimdall.hpp"

namespace heimdall {

auto version() -> std::string_view {
    return "0.1.0";
}

} // namespace heimdall
