#pragma once
#include "../gGfx/gGfx.h"
#include "../gGfx/frames.h"

namespace glib {
namespace rpg {

using glib::gGfx::Point2D;

class Game : public glib::gGfx::Engine
{
public:
    Game();
    ~Game();
    void run();

protected:
    void init();
    int inputHandling();
    void update(float elapsedTime);

private:
    glib::gGfx::EngineGFX* gfx;
    std::string outstr;

    glib::gGfx::Frame* frame = nullptr;

    // TODO: collection of objects to draw
};



} // End of namespace rpg
} // End of namespace glib