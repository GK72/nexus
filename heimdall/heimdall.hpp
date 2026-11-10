/**
 * Part of Nexus Library
 *
 * @brief   Heimdall: the state of every tracked Git repository at a glance.
 *
 * Shared by the `heimdall` and `skald` tools. The model and the survey come
 * in the next parts; see the Heimdall book, doc/heimdall.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <string_view>

namespace heimdall {

/// The version of the tools, printed by --version.
[[nodiscard]] auto version() -> std::string_view;

} // namespace heimdall
