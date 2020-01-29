// **********************************************
// ** gkpro @ 2020-01-29                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include <functional>


#include "datatable.h"
#include "io.h"
#include "pfm.hpp"
#include "utility.h"
#include "arg.h"

#ifdef _WIN32
#include "windows.h"
#endif

#ifdef __linux__

#endif
//#include "libgs.h"              // DLL library

void datatable_test(std::string_view path)
{
    std::cout << "Datatable testing... (path: " << path << ")\n";
    glib::DataTable data(path);
    data.read();
    data.display();
}

struct D256 {
    char x[256];
};

struct D1024 {
    char x[1024];
};

int perfTest() {
    glib::pfm::pfm pfm(100);
    glib::pfm::msrContainer<int, std::vector> msr(10000);
    pfm.add(msr, "Vector measuring");
    pfm.run();
    pfm.print();
    return 0;
}

int run(const glib::ArgParser& args)
{
    //datatable_test(args.get<std::string>("path"));
    perfTest();
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
        glib::printn(e.what());
        return 1;
    }

    if (args.get<bool>("help")) { return 0; }

    try {
        if (args.get<bool>("measure")) {
            std::function<void()> func( [&args]() { run(args); });
            glib::printn(glib::pfm::getMeasureStats(glib::pfm::measure(func, 10), "Test").toString());
        }
        else {
            return run(args);
        }
    }
    catch (const std::runtime_error & e) {
        glib::printn(e.what());
    }
}