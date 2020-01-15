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

void datatable_test(std::string_view path);

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(65001);      // Displaying Unicode characters

    glib::ArgParser args(argc, argv);
    args.add(glib::Arg("path", "Path to something", false, false));
    args.add(glib::Arg("level", "Level"));
    args.add(glib::Arg("debug", "Debug", false, true));

    try {
        args.process();
    }
    catch (const std::runtime_error& e) {
        std::cout << e.what();
        return 1;
    }

    if (args.get("help").getValue() == "true") {
        return 0;
    }

    try {
        datatable_test(args.get("path").getValue());
    }
    catch (const glib::InactiveArgException& e) {
        std::cout << e.what();
    }

}