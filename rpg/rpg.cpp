// **********************************************
// ** gkpro @ 2019-09-22                       **
// **                                          **
// **     Console application entry point      **
// **              ---  RPG  ---               **
// **                                          **
// **                                          **
// **********************************************

#include "rpg.h"

namespace glib {
namespace rpg {


Game::Game(gint width, gint height, gint fontWidth, gint fontHeight)
{
#ifdef _MSC_BUILD
    gfx = new glib::gGfx::EngineConW(this, (int)width, (int)height, (int)fontWidth, (int)fontHeight);
#else
    gfx = new glib::gGfx::EngineCurses(150, 40, 7, 14);
#endif
}

Game::~Game()
{
    delete gfx;
    if (frame) delete frame;
}

void Game::run()
{
    gfx->run();
}

void Game::init()
{
    glib::gGfx::FrameBuilder framebuilder(gfx);
    frames.push_back(framebuilder.createFrame("Main", Point2D(32, 8)));
    frames.push_back(framebuilder.createFrame("Map", Point2D(32, 16), Point2D(50, 0)));
}

int Game::inputHandling()
{
    int inputEventNo = 0;
    if (gfx->getKey(VK_SPACE).isPressed) {
        outstr = "SPACE was pressed";
        ++inputEventNo;
    }

    return inputEventNo;
}


void Game::update(float elapsedTime)
{
    for (const auto& e : frames) {
        e->draw();
    }
}



} // End of namespace rpg
} // End of namespace glib