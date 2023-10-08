#include "gui.h"
#include "pt/pathtracer.h"

#include <nova/color.h>
#include <nova/utils.h>
#include <nova/vec.h>

#include <yui/types.h>
#include <yui/yui.h>

#include <fmt/format.h>

#include <cstdlib>
#include <vector>

/**
 * @brief   Display an image
 *
 * TODO(refact): move into a generic place
 */
GLuint load(class pt::image& img) {
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

auto create_scene() {
    return std::vector<pt::primitive>{
        pt::sphere {
            .position = nova::Vec3f{ 0.0F, 0.0F, -1.0F },
            .radius   = 0.5F,
            .color    = nova::Color{ 0.0F, 0.2F, 1.0F, 1.0F }
        },
        pt::sphere {
            .position = nova::Vec3f{ 0.0F, 100.0F, -20.0F },
            .radius   = 100.0F,
            .color    = nova::Color{ 0.0F, 0.2F, 1.0F, 1.0F }
        },
    };
}

class pt_app : public nxs::gui {
    static constexpr auto WindowSize = nova::Vec2f{ 1280.0F, 720.0F };

    struct app_state {
        pt::camera cam { WindowSize };
        std::vector<pt::primitive> primitives = create_scene();
    };

public:
    pt_app(nxs::app_config cfg)
        : gui(std::move(cfg))
        , m_image(WindowSize)
        , m_pathtracer(m_sync, m_image, m_state.cam, m_state.primitives)
        , m_image_id(load(m_image))
    {
        run();
    }

    ~pt_app() override = default;

private:
    app_state m_state {};
    pt::sync m_sync;

    pt::image m_image;
    pt::pathtracer m_pathtracer;

    GLuint m_image_id;

    void render_ui() override {
        yui::window("Controls")
            .input("Origin x", &m_state.cam.x())
            .input("Origin y", &m_state.cam.y())
            .input("Origin z", &m_state.cam.z())
            .slider("Focal length", &m_state.cam.focal_length(), -10.0F, 10.0F)
            .input("Sampling", &m_pathtracer.config().sampling)
            .button("Rerun pathtracer", [this]() { m_pathtracer.rerun(); })
            .button("Stop pathtracer", [this]() { m_pathtracer.stop(); })
        ;

        yui::window("Camera")
            .image(m_image_id, m_image.dimensions());

        yui::window("Debug")
            .text(fmt::format("Frame time (ms): {:.3f}", nova::to_us(clocks().delta) / 1000));
    }

    /**
     * @brief   Actual rendering is done in the Path Tracer
     */
    void render_gfx() override {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            static_cast<int>(WindowSize.x()),
            static_cast<int>(WindowSize.y()),
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            m_image.data()
        );
    }

    void simulate() override {
        // NO-OP
    }

};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    static constexpr auto Width  = 1920;
    static constexpr auto Height = 1080;
    static constexpr auto FontPath = "";
    static constexpr auto FontSize = 20.0F;

    pt_app({
        .name = "Pathtracer",
        .gui_config = yui::gui_config {
            .window_size = { Width, Height },
            .fontpath = FontPath,
            .fontsize = FontSize
        }
    });

    return EXIT_SUCCESS;
}
