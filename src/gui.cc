#include <yui/types.h>
#include <yui/yui.h>

// #include "chai_bindings.h"

#include "pathtracer.h"
#include "camera.h"

#include <nova/color.h>
#include <nova/utils.h>
#include <nova/vec.h>

#include <fmt/format.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <thread>


namespace nxs {

[[nodiscard]] inline std::chrono::nanoseconds now() {
    return std::chrono::steady_clock().now().time_since_epoch();
}

[[nodiscard]] inline auto to_us(std::chrono::nanoseconds x) {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(x).count());
}

} // namespace nxs

/**
 * @brief   Display an image
 */
GLuint load(class nxs::image& img) {
    // Create a OpenGL texture identifier
    glGenTextures(1, &img.texture_id());
    glBindTexture(GL_TEXTURE_2D, img.texture_id());

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<int>(img.width()), static_cast<int>(img.height()), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());

    return img.texture_id();
}

struct app_config {
    std::string name;
    std::string script_path;
    yui::gui_config gui_config;
};

struct clocks {
    std::chrono::nanoseconds now{ 0 };
    std::chrono::nanoseconds delta{ 0 };
    std::chrono::nanoseconds lastUpdate{ 0 };
};

nxs::camera create_camera() {
    nxs::camera cam;

    cam.dimensions  = { 1280.0F, 720.0F };
    cam.origin      = { 0.0F, 0.0F, 0.0F };
    cam.horizontal  = { cam.dimensions.x() / cam.dimensions.y() * 2.0F, 0.0F, 0.0F };
    cam.vertical    = { 0.0F, 2.0F, 0.0F };
    cam.bottom_left = { cam.origin - cam.horizontal / 2.0F - cam.vertical / 2.0F - nova::Vec3f{ 0.0F, 0.0F, 1.0F } };

    return cam;
}

auto create_scene() {
    return std::vector<nxs::primitive>{
        {
            .position = nova::Vec3f{ 0.0F, 0.0F, 1.0F },
            .radius   = 0.5F,
            .color    = nova::Color{ 0.0F, 0.2F, 1.0F, 1.0F }
        }
    };
}

class app {
    struct app_state {
        float fps_control = 30.0F;
        bool is_frame_limited = false;
        nxs::camera cam = create_camera();
        std::vector<nxs::primitive> primitives = create_scene();
    };

public:
    app(app_config cfg)
        : m_gui(cfg.name, cfg.gui_config)
        , m_config(std::move(cfg))
        , m_image(nova::Vec2f{ 1280.0F, 720.0F })
    {
        // init_scripting();
        m_image_id = load(m_image);

        m_clocks.lastUpdate = nxs::now();

        m_gui.run([this]{
            advance_clock(nxs::now());

            render_ui();
            render_gfx();

            if (m_state.is_frame_limited) {
                using namespace std::chrono_literals;
                const auto sleep = std::min(std::chrono::nanoseconds{ 1s }, std::chrono::nanoseconds(static_cast<long>(1'000'000'000 / m_state.fps_control)) - m_clocks.delta);
                if (sleep > 0s) {
                    std::this_thread::sleep_for(sleep);
                }
            }
        });
    }

private:
    yui::gui m_gui;
    // std::unique_ptr<nxs::chai> m_script;
    app_config m_config;
    app_state m_state;

    nxs::image m_image;
    std::jthread m_render_thread;

    GLuint m_image_id;
    std::string m_logs;
    clocks m_clocks;

    void render_ui() {
        yui::window("Controls")
            .button("Start", [this]{ m_render_thread = std::jthread(&app::read_script, this); })
            .button("Read script", [this]{
                m_logs.clear();
                read_script();
            })
            .input("Frame limit", &m_state.is_frame_limited)
            .input("FPS", &m_state.fps_control)
            .input("Origin x", &m_state.cam.origin.x())
            .input("Origin y", &m_state.cam.origin.y())
            .input("Origin z", &m_state.cam.origin.z())
            .input("Horizontal x", &m_state.cam.horizontal.x())
            .input("Horizontal y", &m_state.cam.horizontal.y())
            .input("Horizontal z", &m_state.cam.horizontal.z())
            .input("Vertical x", &m_state.cam.vertical.x())
            .input("Vertical y", &m_state.cam.vertical.y())
            .input("Vertical z", &m_state.cam.vertical.z())
            .input("bottom_left x", &m_state.cam.bottom_left.x())
            .input("bottom_left y", &m_state.cam.bottom_left.y())
            .input("bottom_left z", &m_state.cam.bottom_left.z())

            .input("sphere x", &m_state.primitives[0].position.x())
            .input("sphere y", &m_state.primitives[0].position.y())
            .input("sphere z", &m_state.primitives[0].position.z())
            .input("sphere r", &m_state.primitives[0].radius)
            .input("sphere color r", &m_state.primitives[0].color.set_r())
            .input("sphere color g", &m_state.primitives[0].color.set_g())
            .input("sphere color b", &m_state.primitives[0].color.set_b())
        ;

        yui::window("Camera")
            .image(m_image_id, m_image.dimensions());

        yui::window("Script debug")
            .text(m_logs);

        yui::window("Debug")
            .text(fmt::format("Frame time (ms): {:.3f}", nxs::to_us(m_clocks.delta) / 1000));
    }

    void render_gfx() {
        pathtracer(m_image, m_state.cam, m_state.primitives);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_image.data());

    }

    // void init_scripting() {
        // m_script = std::make_unique<nxs::chai>();
        // m_script->engine().add(chaiscript::fun(&yui::image::at), "at");
        // m_script->engine().add(chaiscript::fun(&yui::image::width), "width");
        // m_script->engine().add(chaiscript::fun(&yui::image::height), "height");
        // m_script->add_ref(m_image, "img");
    // }

    void read_script() {
        // init_scripting();

        try {
            // m_script->eval_file(m_config.script_path);
        }
        catch (std::exception& ex) {
            m_logs += ex.what();
            m_logs += "\n";
        }
    }

    void advance_clock(std::chrono::nanoseconds now) {
        m_clocks.now = now;
        m_clocks.delta = m_clocks.now - m_clocks.lastUpdate;
        m_clocks.lastUpdate = m_clocks.now;
    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    static constexpr auto Width  = 1920;
    static constexpr auto Height = 1080;
    // static constexpr auto FontPath = "res/ProggyClean.ttf";
    static constexpr auto FontPath = "";
    static constexpr auto FontSize = 20.0F;

    app({
        .name = "Pathtracer",
        .script_path = argc == 2 ? argv[1] : "",
        .gui_config = yui::gui_config {
            .window_size = { Width, Height },
            .fontpath = FontPath,
            .fontsize = FontSize
        }
    });

    return EXIT_SUCCESS;
}
