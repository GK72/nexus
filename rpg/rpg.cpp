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

void Game::run()
{
    gfx->run();
}

void Game::init()
{

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

    Point2D a(10,10);
    Point2D b(30,10);

    // TODO: Do not create new object in every frame
    glib::gGfx::Frame* frame = new glib::gGfx::FrameBasic(gfx, a, b);

    glib::gGfx::FrameContent* c = new glib::gGfx::FrameContentText(frame, std::string("This is a long test string to test word wrap in frame."));
    frame->setContent(c);
    frame->draw();
}



} // End of namespace rpg
} // End of namespace glib