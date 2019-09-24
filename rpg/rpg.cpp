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
EngineGTT::EngineGTT() : EngineConW(100, 40, 7, 14)
{

}

void EngineGTT::init()
{

}

void EngineGTT::update(float elapsedTime)
{
    draw(2, 2, glib::gGfx::FG_WHITE, 'a');
    // TODO: Condition Variable
    // https://ncona.com/2019/04/using-condition-variables-in-cpp/
}



} // End of namespace rpg
} // End of namespace glib