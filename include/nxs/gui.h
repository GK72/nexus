#pragma once

#include <any>
#include <concepts>
#include <map>
#include <string_view>

#include <stdio.h>

#include <spdlog/spdlog.h>
#include <GL/glew.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include "types.h"

// Conversions from-to custom types for avoiding direct use of ImGui implementation.
// Must be defined before `imgui.h` include (alternatively `imconfig.h` could be
// used).

#define IM_VEC2_CLASS_EXTRA                                                     \
    constexpr ImVec2(const nxs::vec2& rhs)                                      \
        : x(rhs.x)                                                              \
        , y(rhs.y)                                                              \
    {}                                                                          \
    operator nxs::vec2() const {                                                \
        return { x, y };                                                        \
    }

#define IM_VEC4_CLASS_EXTRA                                                     \
    constexpr ImVec4(const nxs::vec4& rhs)                                      \
        : x(rhs.x)                                                              \
        , y(rhs.y)                                                              \
        , z(rhs.z)                                                              \
        , w(rhs.w)                                                              \
    {}                                                                          \
    operator nxs::vec4() const {                                                \
        return { x, y, z, w };                                                  \
    }

#include <imgui.h>
#include "bindings/imgui_impl_glfw.h"         // Conan import
#include "bindings/imgui_impl_opengl3.h"      // Conan import

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#include "utils.h"

namespace nxs {

struct ui_state {
    bool isDirty = true;
    std::any state;
};

struct header_column {
    std::string name;
    float width;
};

/**
 * @brief   Fundamental UI element holding other UI elements
 *
 * Displaying text, drawing on it.
 */
class ui_window {
public:
    ui_window(const std::string& title, ui_state& cache)
        : m_cache(cache)
    {
        ImGui::Begin(title.c_str());
    }

    ~ui_window() {
        ImGui::End();
    }

    ui_window(const ui_window&)            = delete;
    ui_window(ui_window&&)                 = delete;
    ui_window& operator=(const ui_window&) = delete;
    ui_window& operator=(ui_window&&)      = delete;

    /**
     * @brief   Create a button which calls the `callback` upon clicking
     */
    ui_window& button(const std::string& name, std::invocable auto&& callback) {
        if (ImGui::Button(name.c_str())) {
            callback();
        }
        return *this;
    }

    /**
     * @brief   Display text as is
     */
    ui_window& text(const std::string& msg) {
        ImGui::Text("%s", msg.c_str());
        return *this;
    }

    /**
     * @brief   Display as formatted text
     */
    template <class ...Args>
    ui_window& text(const std::string& fmt, Args&&... args) {
        ImGui::Text("%s", fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args))...).c_str());
        return *this;
    }

    /**
     * @brief   Create an input box
     */
    template <class T>
    ui_window& input(const std::string& label, T* value) {
        if constexpr (std::is_same_v<T, int>) {
            ImGui::InputInt(label.c_str(), value);
        } else if constexpr (std::is_same_v<T, float>) {
            ImGui::InputFloat(label.c_str(), value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            ImGui::InputText(
                label.c_str(),
                value->data(),
                value->capacity() + 1,
                ImGuiInputTextFlags_CallbackResize,
                [](ImGuiInputTextCallbackData* data) -> int {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                        std::string* str = static_cast<std::string*>(data->UserData);
                        str->resize(data->BufTextLen);
                        data->Buf = str->data();
                    }
                    return 0;
                },
            value
            );
        }
        else {
            static_assert(nxs::dependent_false_v<T>, "Only the following types are supported for input: int, float, std::string");
        }
        return *this;
    }

    /**
     * @brief   Create a checkbox
     */
    ui_window& checkbox(const std::string& label, bool* value) {
        ImGui::Checkbox(label.c_str(), value);
        return *this;
    }

    /**
     * @brief   Draw a line
     *
     * Color is byte packed into a single 32 bit value
     */
    ui_window& line(const nxs::vec2& a, const nxs::vec2& b, const nxs::vec4& color, float thickness = 1.0f) {
        ImGui::GetForegroundDrawList()->AddLine(winPos(a), winPos(b), nxs::color(color), thickness);
        return *this;
    }

    /**
     * @brief   Draw a circle
     *
     * Color is byte packed into a single 32 bit value
     */
    ui_window& circle(const nxs::vec2& center, float radius, const nxs::vec4& color) {
        ImGui::GetForegroundDrawList()->AddCircleFilled(winPos(center), radius, nxs::color(color));
        return *this;
    }

    /**
     * @brief   Draw a plotline
     */
    ui_window& plotLine(const std::string& title, const std::string& overlayText, const auto& range, const nxs::vec2& plotSize = { 300, 50 }, int offset = 0) {
        ImGui::PlotLines(
            title.c_str(),
            range.data(),
            static_cast<int>(range.size()),
            offset,
            overlayText.c_str(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            plotSize
        );
        return *this;
    }

    /**
     * @brief   Draw a plotline from a ring buffer
     *
     * with the actual value displayed on the overlay.
     */
    ui_window& plotLine(const std::string& title, std::string_view overlayFmt, const nxs::ring<float>& range, const nxs::vec2& plotSize = { 300, 50 }) {
        return plotLine(
            title,
            fmt::vformat(overlayFmt, fmt::make_format_args(range.back())),
            range,
            plotSize,
            static_cast<int>(range.offset())
        );
    }

    /**
     * @brief   Create a table
     *
     * Preparing into a table format - which is a list of strings - is automatically
     * done with a `toTable` function call and will be cached avoiding constructing
     * the vector every frame.
     */
    template <class T>
        requires requires (T a ) {
            { toTable(a) } -> std::convertible_to<nxs::vector<std::string>>;
        }
    ui_window& table(const std::string& title, const T& values, const nxs::vector<nxs::header_column>& header, const nxs::vec2& tableSize = { 0, 0 }) {
        using State_Type = decltype(toTable(values));

        if (m_cache.isDirty) {
            m_cache.isDirty = false;
            m_cache.state = std::make_any<State_Type>(toTable(values));
        }

        ImGui::BeginTable(title.c_str(), static_cast<int>(header.size()), 0, tableSize);
        for (const auto& col : header) {
            ImGui::TableSetupColumn(col.name.c_str(), ImGuiTableColumnFlags_WidthFixed, col.width);
        }
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();

        for (const auto& x : std::any_cast<State_Type&>(m_cache.state)) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", x.c_str());
        }

        ImGui::EndTable();
        return *this;
    }

private:
    ui_state& m_cache;

    /**
     * @brief   Obtain the relative position by some offset
     */
    [[nodiscard]] static nxs::vec2 winPos(nxs::vec2 p, nxs::vec2 offset = { 5, 30 }) noexcept {
        return { p + ImGui::GetWindowPos() + offset };
    }
};

class Menu {
public:
    Menu(const std::string& name)
        : m_active(ImGui::BeginMenu(name.c_str()))
    {}

    ~Menu() {
        if (m_active) {
            ImGui::EndMenu();
        }
    }

    Menu(const Menu&) = delete;
    Menu(Menu&&) = delete;
    Menu& operator=(const Menu&) = delete;
    Menu& operator=(Menu&&) = delete;

    Menu& item(const std::string& name, bool* selected, const std::string& shortcut = "") {
        if (m_active) {
            ImGui::MenuItem(name.c_str(), shortcut == "" ? nullptr : shortcut.c_str(), selected);
        }
        return *this;
    }

private:
    bool m_active;
};

class MainMenu {
public:
    MainMenu()
        : m_active(ImGui::BeginMainMenuBar())
    {}

    ~MainMenu() {
        if (m_active) {
            ImGui::EndMainMenuBar();
        }
    }

    MainMenu(const MainMenu&) = delete;
    MainMenu(MainMenu&&) = delete;
    MainMenu& operator=(const MainMenu&) = delete;
    MainMenu& operator=(MainMenu&&) = delete;

    Menu menu(const std::string& name) {
        return Menu(name);
    }

private:
    bool m_active;
};


/**
 * @brief   Wrapper class around GLFW (OpenGL3 implementation) and ImGUI
 */
class gui {
public:
    gui(const std::string& title, nxs::vec2 pos);
    ~gui();

    gui(const gui&)             = delete;
    gui(gui&& rhs) noexcept;
    gui& operator=(const gui&)  = delete;
    gui& operator=(gui&&)       = delete;

    void run(std::invocable auto callback);

    MainMenu mainMenu() {
        return MainMenu();
    }

    /**
     * @brief   Create a window with cache
     *
     * Converting data to the proper format should be abstracted away from the client code; it
     * must be done automatically with the provided interface (see more details at the member
     * functions of the `ui_window`).
     *
     * Data should be converted to the appropriate type only when it is needed
     * (hence the caching).
     */
    ui_window window(const std::string& name) {
        if (not m_caches.contains(name)) {
            m_caches[name] = ui_state{};
        }
        return ui_window(name, m_caches[name]);
    }

    [[nodiscard]] auto dimensions() const noexcept {
        return m_dimensions;
    }

private:
    GLFWwindow* m_window = nullptr;
    std::string_view m_glslVersion;
    nxs::vec2 m_dimensions;

    std::map<std::string, ui_state> m_caches;       // TODO(perf,data-structure): flatmap

    static void clearColor(const vec4& color);
    static std::string_view GLSLVersion();
};

/**
 * @brief   Event-loop
 */
void gui::run(std::invocable auto callback) {
    while (not glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        clearColor(colors::black);

        callback();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

inline void gui::clearColor(const vec4& color) {
    glClearColor(
        color.x * color.w,
        color.y * color.w,
        color.z * color.w,
        color.w
    );

    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace nxs
