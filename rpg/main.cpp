// **********************************************
// ** gkpro @ 2019-09-22                       **
// **                                          **
// **     Console application entry point      **
// **              ---  RPG  ---               **
// **                                          **
// **                                          **
// **********************************************

#include "gGfx.h"

int main()
{
    glib::gGfx::EngineGFX* gfx = new glib::gGfx::EngineCurses();
    gfx->run();

}