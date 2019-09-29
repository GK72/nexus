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
EngineGTT::EngineGTT() : EngineConW(150, 40, 7, 14)
{

}

void EngineGTT::init()
{

}

int EngineGTT::input()
{
    int inputEventNo = 0;
    if (getKey(VK_SPACE).isPressed) {
        outstr = "SPACE was pressed";
        ++inputEventNo;
    }

    if (getKey(VK_RETURN).isPressed) {
        outstr = "RETURN was pressed";
        ++inputEventNo;
    }

    return inputEventNo;
}



void EngineGTT::update(float elapsedTime)
{
    setCurPosX(0);
    setCurPosY(0);

    printn("Testing...");
    printn("=======================================");
    printn(std::to_string(elapsedTime));
    printn();
    printn(outstr);
    // TODO: Condition Variable
    // https://ncona.com/2019/04/using-condition-variables-in-cpp/
}



} // End of namespace rpg
} // End of namespace glib