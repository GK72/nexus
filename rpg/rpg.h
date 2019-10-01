#pragma once
#include "gGfx.h"
#include "frames.h"

namespace glib {
namespace rpg {

using glib::gGfx::Point2D;

class Game : public glib::gGfx::Engine
{
public:
    Game();
    void run();

protected:
    void init();
    int inputHandling();
    void update(float elapsedTime);

private:
    glib::gGfx::EngineGFX* gfx;
    std::string outstr;
};



} // End of namespace rpg
} // End of namespace glib