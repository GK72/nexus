/*
 * gkpro @ 2021-11-06
 *   Nexus Library
 *   GUI Wrapper
 */

#pragma once

#include <concepts>
#include <memory>
#include <string_view>

#include <stdio.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "chaiscript.hpp"
#include "spdlog.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

namespace nxs {

struct Coords {
    int x;
    int y;
};

/**
 * @brief   Wrapper class around ImGUI (OpenGL3 implementation)
 */
class GUI {
public:
    GUI(Coords coords);
    ~GUI();

    GUI(const GUI&)             = delete;
    GUI(const GUI&&)            = delete;
    GUI& operator=(const GUI&)  = delete;
    GUI& operator=(GUI&&)       = delete;

    void run(std::invocable auto callback);

private:
    GLFWwindow* m_window = nullptr;
    std::string_view m_glslVersion;

    static void clearColor(const ImVec4& color);
    static std::string_view GLSLVersion();
};

inline GUI::GUI(Coords coords)
    : m_glslVersion(GLSLVersion())
{
    glfwSetErrorCallback([](int err, const char* msg) { spdlog::error("{} {}", err, msg); });

    if (glfwInit() == 0) {
        throw std::runtime_error("Failed to initialize GUI");
    }

    m_window = glfwCreateWindow(coords.x, coords.y, "Nexus GUI tool", nullptr, nullptr);
    if (m_window == nullptr) {
        throw std::runtime_error("Failed to initialize window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(m_glslVersion.data());
}

inline GUI::~GUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

/**
 * @brief   Event-loop
 */
void GUI::run(std::invocable auto callback) {
    while (glfwWindowShouldClose(m_window) == 0) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto& state = callback();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);

        glViewport(0, 0, display_w, display_h);
        clearColor(state.colorBackground);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

inline void GUI::clearColor(const ImVec4& color) {
    glClearColor(
        color.x * color.w,
        color.y * color.w,
        color.z * color.w,
        color.w
    );

    glClear(GL_COLOR_BUFFER_BIT);
}

inline std::string_view GUI::GLSLVersion() {
    #if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    return "#version 100";

    #elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    return "#version 150";

    #else
    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    return "#version 130";
    #endif
}

inline auto ChaiScriptImGUIBindings() {
    namespace cs = chaiscript;
    auto module = std::make_shared<cs::Module>();

    auto addFn = [&module](const auto& name, auto func) {
        module->add(chaiscript::fun(func), name);
    };

    // module.add(cs::fun([](const std::string& text) { ImGui::Text(text.c_str()); }), "ImGui_Text");

    addFn("GuiText", [](const std::string& text) { ImGui::Text(text.c_str()); });
    addFn("GuiBegin", [](const std::string& text) { ImGui::Begin(text.c_str()); });
    addFn("GuiEnd", []() { ImGui::End(); });
    addFn("GuiCheckbox", [](const std::string& text, bool* b) { return ImGui::Checkbox(text.c_str(), b); });

    return module;
}

} // namespace nxs
