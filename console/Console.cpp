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
    glib::pfm::msrContainer<int, std::vector> msr(100000);
    pfm.add(msr, "Vector measuring");
    pfm.run();
    pfm.print();
    return 0;
}

int thread() {
    auto time_start = std::chrono::high_resolution_clock::now();

    glib::ThreadPool tp;

    for (int i = 0; i < 2; ++i) {
        tp.add([&tp, time_start, i] {
            std::chrono::milliseconds elapsed{ 0 };
            while (elapsed < std::chrono::milliseconds(3000)) {
                auto time2 = std::chrono::high_resolution_clock::now();
                elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time_start);
                std::string msg = glib::joinStr(" ", "Working...", elapsed);
                tp.message(msg, i + 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            });
    }

    tp.joinAll();

    return 0;
}

int func(int x) {
    return x + 1;
}

int callable() {
    //auto f = glib::Callable(func);
    //auto f = [](auto x) { return x + 10; };
    //auto b = glib::BoxedCallable(f);
    //glib::printn(b(1));

    return 0;
}


int run(const glib::ArgParser& args)
{
    //datatable_test(args.get<std::string>("path"));
    //perfTest();
    thread();
    //callable();
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
