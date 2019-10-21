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
#include "game.h"

#ifdef __linux__
#include "curses.h"
#endif
//#include "libgs.h"              // DLL library

int main()
{
    rts::Game* game = new rts::Game(192, 108, 7, 14);
    game->run();

}