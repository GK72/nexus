#pragma once

#include <nova/utils.h>

#include <yui/types.h>
#include <yui/yui.h>

#include <chrono>
#include <string>
#include <thread>

namespace nxs {

struct app_config {
    std::string name;
    yui::gui_config gui_config;
};

struct clocks {
    std::chrono::nanoseconds now{ 0 };
    std::chrono::nanoseconds delta{ 0 };
    std::chrono::nanoseconds lastUpdate{ 0 };
};

class gui {
public:
    gui(app_config cfg)
        : m_gui(cfg.name, cfg.gui_config)
        , m_config(std::move(cfg))
    {}

    virtual ~gui() = default;

    virtual void simulate() = 0;
    virtual void render_ui() = 0;
    virtual void render_gfx() = 0;

    void run() {
        m_clocks.lastUpdate = nova::now();

        m_gui.run([this]{
            advance_clock(nova::now());
            render_ui();
            simulate();
            render_gfx();
        });
    }

    [[nodiscard]] const struct clocks& clocks() const noexcept { return m_clocks; }

private:
    yui::gui m_gui;
    app_config m_config;

    std::jthread m_render_thread;
    std::string m_logs;
    struct clocks m_clocks;

    void advance_clock(std::chrono::nanoseconds now) {
        m_clocks.now = now;
        m_clocks.delta = m_clocks.now - m_clocks.lastUpdate;
        m_clocks.lastUpdate = m_clocks.now;
    }

    void event_loop() {
        advance_clock(nova::now());
        render_ui();
        render_gfx();
    }
};

} // namespace nxs
