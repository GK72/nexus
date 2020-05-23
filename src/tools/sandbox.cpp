/*
 * gkpro @ 2020-05-23
 *   Sandbox for G-Library
 *   Template for using ArgParser
 */

#include "arg.h"
#include "utility.h"
#include "pfm.hpp"

int run(const glib::ArgParser& args)
{
    glib::print("Empty sandbox");
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
    glib::ArgParser args = argParsing(argc, argv);

    try { args.process(); }
    catch (const std::runtime_error& e) {
        glib::print(e.what());
        return 1;
    }

    if (args.get<bool>("--help")) { return 0; }

    try {
        if (args.get<bool>("measure")) {
            std::function<void()> func( [&args]() { run(args); });
            glib::print(glib::pfm::getMeasureStats(glib::pfm::measure(func, 10), "Test").toString());
        }
        else {
            return run(args);
        }
    }
    catch (const std::runtime_error & e) {
        glib::print(e.what());
    }
}

