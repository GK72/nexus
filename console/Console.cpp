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
//#include "libgs.h"              // DLL library



int main()
{
    std::string ip("fe80:0000:0000:0000:fc30:35fb:e172:4230");
    std::cout << glib::ipv6Formatter(ip);


	return 0;
}

