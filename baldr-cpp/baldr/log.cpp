#include <baldr/log.hpp>
#include <baldr/progress.hpp>

#include <libnova/log.hpp>

#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>

#include <cstdlib>
#include <cstdio>
#include <memory>
#include <mutex>
#include <unistd.h>

namespace baldr::log {

namespace {

/**
 * @brief   A spdlog sink that renders log records as in-place progress lines.
 *
 * Normal levels (`trace`/`debug`/`info`/`warn`) keep a rolling window of the
 * latest lines visible via `progress::msg`. Only `error`/`critical` records
 * are treated as the special case: they trigger `progress::failure`, which
 * dumps the entire buffered log history so the user can see everything that
 * led up to the failure.
 *
 * This class is deliberately self-contained so it can later be backported
 * into `libnova` as a reusable sink.
 */
class progress_sink : public spdlog::sinks::base_sink<std::mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::string line = fmt::to_string(formatted);
        while (not line.empty() and (line.back() == '\n' or line.back() == '\r')) {
            line.pop_back();
        }

        if (msg.level == spdlog::level::err or msg.level == spdlog::level::critical) {
            m_progress.failure(line);
        } else {
            m_progress.msg(line);
        }
    }

    void flush_() override {
        std::fflush(stdout);
    }

private:
    baldr::progress m_progress;
};

} // namespace

auto resolve_mode(std::optional<mode> cli_override) -> mode {
    if (cli_override) {
        return *cli_override;
    }

    if (const char* env = std::getenv("BALDR_LOG_MODE"); env != nullptr) {
        std::string value = env;
        if (value == "standard") {
            return mode::standard;
        }
        if (value == "progress") {
            return mode::progress;
        }
        nova::log::warn("Invalid BALDR_LOG_MODE value '{}', falling back to 'standard'", value);
        return mode::standard;
    }

    if (isatty(fileno(stderr)) != 0) {
        return mode::progress;
    }

    return mode::standard;
}

void init(mode m, const std::string& name) {
    nova::log::load_env_levels();
    spdlog::drop(name);

    if (m == mode::progress) {
        spdlog::set_default_logger(spdlog::create<progress_sink>(name));
    } else {
        spdlog::set_default_logger(spdlog::create<spdlog::sinks::ansicolor_stderr_sink_mt>(name));
    }

    spdlog::default_logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%f %z] [%n @%t] %^[%l]%$ %v");
}

} // namespace baldr::log
