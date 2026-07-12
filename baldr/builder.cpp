#include <baldr/builder.hpp>
#include <baldr/command.hpp>

#include <libnxs/line_reader.hpp>
#include <libnxs/rlog.hpp>
#include <libnova/error.hpp>
#include <libnova/log.hpp>

#include <fmt/format.h>

namespace baldr {

builder::builder(std::string project_dir, std::vector<std::string> build_command)
    : m_project_dir(std::move(project_dir))
    , m_build_command(std::move(build_command))
{}

void builder::build() const {
    nova::log::debug("Building in '{}'...", m_project_dir);

    // TODO(feat): Build types
    auto cmd = command{ m_build_command, {}, m_project_dir };
    cmd.run();

    nxs::line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), not chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();

    int code = cmd.wait();
    if (code == 0) {
        nxs::rlog::success("Build successful.");
    } else {
        nxs::rlog::failure(fmt::format("Build failed (exit code {}).", code));
        throw nova::exception("Build failed (exit code {}).", code);
    }
}

void builder::run(const std::string& target) const {
    // TODO(feat): CMake projects will need a discovery mechanism.
    std::string exe_path = fmt::format("./{}", target);
    nova::log::debug("Running '{}' in '{}'...", exe_path, m_project_dir);

    auto cmd = command{ { exe_path }, {}, m_project_dir, /*interactive=*/true };
    cmd.run();

    if (int code = cmd.wait(); code != 0) {
        throw nova::exception("'{}' exited with code {}.", exe_path, code);
    }
}

} // namespace baldr
