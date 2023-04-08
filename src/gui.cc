#include "nxs/gui.h"

namespace {
    void logOpenGLversion() {
        const auto version = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        spdlog::info("OpenGL Version: {}", version);
    }
}

namespace nxs {

gui::gui(const std::string& title, nxs::vec2 pos)
    : m_glslVersion(GLSLVersion())
    , m_dimensions(pos)
{
    glfwSetErrorCallback([](int err, const char* msg) { spdlog::error("{} {}", err, msg); });

    if (glfwInit() == 0) {
        throw std::runtime_error("Failed to initialize GUI");
    }

    m_window = glfwCreateWindow(static_cast<int>(pos.x), static_cast<int>(pos.y), title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        throw std::runtime_error("Failed to initialize window");
    }

    glfwMakeContextCurrent(m_window);

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    logOpenGLversion();

    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.Fonts->AddFontFromFileTTF("res/ProggyClean.ttf", 20);

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(m_glslVersion.data());
}

gui::gui(gui&& rhs) noexcept {
    m_window = rhs.m_window;
    m_glslVersion = rhs.m_glslVersion;
    m_dimensions = rhs.m_dimensions;

    rhs.m_window = nullptr;
}

gui::~gui() {
    if (m_window == nullptr) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

std::string_view gui::GLSLVersion() {
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

} // namespace nxs
