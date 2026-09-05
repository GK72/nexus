#include <baldr/batch.hpp>
#include <baldr/exec.hpp>

#include <libnxs/rlog.hpp>

#include <libnova/error.hpp>
#include <libnova/log.hpp>
#include <libnova/utils.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <chrono>

namespace fs = std::filesystem;

namespace baldr {

batch_runner::batch_runner(std::string project_dir, batch_config cfg)
    : m_project_dir(std::move(project_dir))
    , m_cfg(std::move(cfg))
{}

void batch_runner::run_stage(const std::string& name, const std::map<std::string, std::string>& params) {
    auto it = m_cfg.stages.find(name);
    if (it == m_cfg.stages.end()) {
        throw nova::exception("Unknown batch stage `{}`", name);
    }
    const auto& stage = it->second;

    for (const auto& [key, value] : params) {
        if (std::ranges::find(stage.parameters, key) == stage.parameters.end()) {
            throw nova::exception("Stage `{}` doesn't declare a `{}` parameter", name, key);
        }
    }
    for (const auto& param : stage.parameters) {
        if (not params.contains(param)) {
            throw nova::exception("Stage `{}` requires parameter `{}` (use -P/--param {}=<value>)", name, param, param);
        }
    }

    auto env = m_cfg.env;
    env["BALDR_ENV_WORKING_DIR"] = fs::absolute(m_project_dir).string();
    for (const auto& [key, value] : params) {
        env.insert_or_assign(key, value);
    }

    nova::log::info("Running batch stage `{}`: {}", name, stage.cmd);

    const auto start = nova::steady_now();
    const auto status = run_streamed({ "/bin/sh", "-c", stage.cmd }, m_project_dir, env);
    const std::chrono::duration<double> elapsed = nova::steady_now() - start;

    const auto stats = [&]() {
        const std::chrono::duration<double> cpu_time = status.usage().user_time + status.usage().system_time;
        const double peak_rss_mb = static_cast<double>(status.usage().peak_rss_kb) / 1024.0;
        return fmt::format("{:.3f}s (cpu {:.3f}s, peak {:.1f} MB)", elapsed.count(), cpu_time.count(), peak_rss_mb);
    }();

    if (status.success()) {
        nxs::rlog::success(fmt::format("Stage `{}` finished successfully in {}", name, stats));
        return;
    }

    if (status.interrupted()) {
        nxs::rlog::success(fmt::format("Stage `{}` interrupted after {}", name, stats));
        throw nova::exception("Stage `{}` interrupted.", name);
    }

    throw nova::exception("Stage `{}` failed: {} ({})", name, status.describe(), stats);
}

void batch_runner::run_scenario(const std::string& name) {
    auto it = m_cfg.scenarios.find(name);
    if (it == m_cfg.scenarios.end()) {
        throw nova::exception("Unknown batch scenario `{}`", name);
    }

    nova::log::info("Running batch scenario `{}` ({} step(s))", name, it->second.size());

    for (const auto& step : it->second) {
        run_stage(step.stage, step.overrides);
    }

    nova::log::info("Scenario `{}` finished successfully", name);
}

} // namespace baldr
