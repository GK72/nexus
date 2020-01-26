// **********************************************
// ** gkpro @ 2019-09-17                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include <functional>


#include "utility.h"
#include "io.h"
#include "arg.h"

#ifdef _WIN32
#include "windows.h"
#endif

#ifdef __linux__
//#include "curses.h"
#endif
//#include "libgs.h"              // DLL library

void datatable_test(std::string_view path);

int run(const glib::ArgParser& args)
{
    datatable_test(args.get<std::string>("path"));
    return 0;
}

glib::ArgParser argParsing(int argc, char* argv[])
{
    glib::ArgParser args(argc, argv);
    glib::ArgFactory factory(&args);
    factory.add("path", "Path to something");
    factory.addFlag("debug", "Debug flag");
    factory.addFlag("measure", "Measuring flag");
    return args;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);      // Displaying Unicode characters
#endif

    glib::ArgParser args = argParsing(argc, argv);

    try { args.process(); }
    catch (const std::runtime_error& e) {
        glib::print(e.what());
        return 1;
    }

    if (args.get<bool>("help")) { return 0; }

    try {
        if (args.get<bool>("measure")) {
            //std::function<int(glib::ArgParser)> func = std::bind(run, args);
            auto func = std::bind(run, args);
            glib::print("", glib::getMeasureStats(glib::measure(func, 10), "Test"));
        }
        else {
            return run(args);
        }
    }
    catch (const std::runtime_error & e) {
        glib::print(e.what());
    }
}