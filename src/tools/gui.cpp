#include "gui.h"

const auto darkCyan = ImVec4(0.021, 0.066, 0.098, 1.0);

struct AppState {
    ImVec4 colorBackground = darkCyan;
    bool showDemoWindow = true;
};

int main(int argc, const char* args[]) {
    const auto guiScript = [argc, args]{ return argc == 2 ? args[1] : "chaiscripts/guiScript.chai"; }();      // NOLINT

    constexpr auto width  = 1280;
    constexpr auto height = 720;
    auto gui = nxs::GUI(nxs::Coords{ width, height });

    AppState state;

    chaiscript::ChaiScript chai;
    chai.add(nxs::ChaiScriptImGUIBindings());
    chai.add(chaiscript::fun(&AppState::showDemoWindow), "showDemoWindow");
    chai.add(chaiscript::var(&state), "state");

    gui.run([&state, &guiScript, &chai]() mutable {
        if (state.showDemoWindow) {
            ImGui::ShowDemoWindow(&state.showDemoWindow);
        }

        chai.eval_file(guiScript);

        return state;
    });
}
