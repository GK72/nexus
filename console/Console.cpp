// **********************************************
// ** gkpro @ 2019-09-17                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include "gmath.h"
#include "gGfx.h"
#include "utility.h"
#include "rpg.h"

#ifdef __linux__
#include "curses.h"
#endif
//#include "libgs.h"              // DLL library

int main()
{
#ifdef __linux__
    glib::gGfx::EngineGFX* gfx = new glib::gGfx::EngineCurses();
#elif defined(_MSC_BUILD)
    glib::gGfx::EngineGFX* gfx = new glib::rpg::EngineGTT();
#endif
    gfx->run();
}
