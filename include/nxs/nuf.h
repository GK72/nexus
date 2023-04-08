#pragma once

#include <array>
#include <chrono>
#include <list>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "nxs/gui.h"
#include "nxs/utils.h"

namespace nuf {

struct Clocks {
    std::chrono::nanoseconds now{ 0 };
    std::chrono::nanoseconds delta{ 0 };
    std::chrono::nanoseconds lastUpdate{ 0 };
};

struct History {
    nxs::ring<float> renderTimesMs{ 600 };
};

struct UIState {
    int objects = 1;
    bool visible = true;
};

class Game {
public:
    Game(nxs::mem* memoryResource, nxs::gui&& gui);
    void start();

private:
    nxs::mem* m_mem;
    nxs::gui m_gui;

    UIState m_uiState;
    Clocks m_clocks;
    History m_history;

    void config();
    void update();
    void renderUI();

    void advanceClock(std::chrono::nanoseconds now);
};

inline Game::Game(nxs::mem* memoryResource, nxs::gui&& gui)
    : m_mem(memoryResource)
    , m_gui(std::move(gui))
{}

inline void Game::config() {
}

inline void Game::start() {
    config();

    m_clocks.lastUpdate = nxs::now();

    m_gui.run([this]() {
        advanceClock(nxs::now());

        update();
        m_history.renderTimesMs.push_back(static_cast<float>(nxs::toUs(nxs::now() - m_clocks.now) / 1000));
        renderUI();
    });
}

inline void Game::advanceClock(std::chrono::nanoseconds now) {
    m_clocks.now = now;
    m_clocks.delta = m_clocks.now - m_clocks.lastUpdate;
    m_clocks.lastUpdate = m_clocks.now;
}

inline void Game::update() {
}

inline void Game::renderUI() {
    m_gui.window("Debug")
        .text(fmt::format("Frame time (ms): {:.3f}", nxs::toUs(m_clocks.delta) / 1000))
        .plotLine("Render times", "{:.3f} ms", m_history.renderTimesMs)
        #ifndef NDEBUG
        .text(fmt::format("Total allocations: {}", m_mem->totalAllocations()))
        #endif
        ;
}

} // namespace nuf
