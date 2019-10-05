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


Game::Game()
{
#ifdef _MSC_BUILD
    gfx = new glib::gGfx::EngineConW(this, 150, 40, 7, 14);
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
    frame = framebuilder.createFrame();
}

int Game::inputHandling()
{
    int inputEventNo = 0;
    if (gfx->getKey(VK_SPACE).isPressed) {
        outstr = "SPACE was pressed";
        ++inputEventNo;
    }

    if (gfx->getKey(VK_RETURN).isPressed) {
        outstr = "RETURN was pressed";
        ++inputEventNo;
    }

    return inputEventNo;
}


void Game::update(float elapsedTime)
{
    gfx->setCurPosX(0);
    gfx->setCurPosY(0);

    gfx->printn("Testing...");
    
    gfx->printn("=======================================");
    gfx->printn(std::to_string(elapsedTime));
    gfx->printn();
    gfx->printn(outstr);

    frame->draw();
}



} // End of namespace rpg
} // End of namespace glib