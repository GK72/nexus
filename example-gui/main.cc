#include <exception>
#include <memory_resource>

#include "nxs/nuf.h"
#include "nxs/utils.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* args[]) {
    constexpr auto width  = 1920;
    constexpr auto height = 1080;

    auto mem = nxs::mem();
    std::pmr::set_default_resource(&mem);

    try {
        auto game = nuf::Game(&mem, nxs::gui("Abyss", nxs::vec2{ width, height }));
        game.start();
    }
    catch (const nxs::bad_expected_access& ex) {
        spdlog::error("{}", ex.error());
    }
    catch (const std::exception& ex) {
        spdlog::error("{}", ex.what());
    }
}
