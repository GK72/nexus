/**
 * Part of Nexus Library
 *
 * @brief   Runs a program to completion and captures its output.
 *
 * Unlike `nxs::command`, which drives a child on a pseudo-terminal, this keeps
 * standard output and standard error apart and byte-exact, as parsing Git's
 * porcelain output needs.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-10
 * @updated     2026-11-10
 * @status      experimental
 * @reviewed    none
 * SPDX-License-Identifier: Unlicense
 */

#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace heimdall {

struct process_options {
    std::string directory;                                      ///< Working directory; empty: the current one
    std::vector<std::pair<std::string, std::string>> env;       ///< Added to, or replacing, the environment
    std::chrono::milliseconds timeout = std::chrono::seconds(30);
    bool new_session = false;   ///< No controlling terminal, so the child can never prompt
};

struct process_result {
    int status = -1;            ///< Exit status; 128 + signal if killed; 127 if it could not start
    std::string out;
    std::string err;
    bool timed_out = false;

    [[nodiscard]] auto success() const -> bool { return status == 0 and not timed_out; }
};

/**
 * @brief   Runs `args` (the program is looked up in `PATH`) with standard input
 *          from `/dev/null`, and waits for it.
 *
 * A child that outlives `timeout` is killed, with its process group when it
 * runs in a session of its own.
 */
[[nodiscard]] auto run_process(const std::vector<std::string>& args, const process_options& options = {})
    -> process_result;

} // namespace heimdall
