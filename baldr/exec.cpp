#include <baldr/exec.hpp>
#include <baldr/signal.hpp>

#include <libnxs/line_reader.hpp>
#include <libnxs/rlog.hpp>
#include <libnova/log.hpp>

#include <csignal>

namespace baldr {

auto run_streamed(
        const std::vector<std::string>& args,
        const std::string& working_directory,
        const std::map<std::string, std::string>& env
) -> command::exit_status {
    auto resolved_env = env;
    resolved_env.emplace(nxs::rlog::EnvLogMode, "plain");

    auto cmd = command{ args, resolved_env, working_directory };
    cmd.run();
    auto watch = signal_handler::scoped_watch{ SIGINT, cmd.pid() };

    nxs::line_reader lines([](std::string& line) { nova::log::info("{}", line); });
    std::string chunk;
    while (chunk = cmd.poll(), not chunk.empty()) {
        lines.feed(chunk);
    }
    lines.feed_eof();

    return cmd.wait();
}

} // namespace baldr
