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
    glib::gGfx::EngineGFX* game = new glib::rpg::EngineGTT();
    game->run();
}