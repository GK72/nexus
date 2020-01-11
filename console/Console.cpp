// **********************************************
// ** gkpro @ 2019-09-17                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include <fstream>
#include "windows.h"
#include "utility.h"
#include "io.h"
#include "arg.h"

#ifdef __linux__
#include "curses.h"
#endif
//#include "libgs.h"              // DLL library

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(65001);      // Displaying Unicode characters

    glib::ArgParser args(argc, argv);
    args.add(glib::Arg("path", "Path to something", false, false));
    args.add(glib::Arg("level", "Level", false, false));
    args.process();
    std::cout << args.get("path").value << '\n';
    std::cout << args.get("level").value << '\n';

}