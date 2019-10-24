// **********************************************
// ** gkpro @ 2019-09-17                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include <fstream>
//#include "gmath.h"
//#include "gGfx.h"
#include "utility.h"
#include "io.h"
//#include "game.h"

#ifdef __linux__
#include "curses.h"
#endif
//#include "libgs.h"              // DLL library

int main()
{
    //rts::Game* game = new rts::Game(192, 108, 7, 14);
    //game->run();

    glib::ParserJSON json(R"#(D:\dev\data\houses.json)#");
    auto data = json.read();
    for (const auto& e : data) {
        for (const auto& m : e) {
            std::cout << m.first << ": " << std::any_cast<std::string>(m.second) << '\n';
        }
    }

    //std::cout << json.readToken() << '\n';

}