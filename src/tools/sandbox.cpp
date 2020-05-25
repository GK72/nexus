/*
 * gkpro @ 2020-05-23
 *   Sandbox for Nexus Library
 *   Template for using ArgParser
 */

#include "arg.h"
#include "utility.h"

int run(const nxs::ArgParser& args)
{
    nxs::print("Empty sandbox");
    return 0;
}

nxs::ArgParser argParsing(int argc, char* argv[])
{
    nxs::ArgParser args(argc, argv);
    nxs::ArgFactory factory(&args);
    factory.add("path", "Path to something");
    factory.addFlag("debug", "Debug flag");
    factory.addFlag("measure", "Measuring flag");
    return args;
}

int main(int argc, char* argv[])
{
    nxs::ArgParser args = argParsing(argc, argv);

    try { args.process(); }
    catch (const std::runtime_error& e) {
        nxs::print(e.what());
        return 1;
    }

    if (args.get<bool>("--help")) { return 0; }

    try {
        return run(args);
    }
    catch (const std::runtime_error & e) {
        nxs::print(e.what());
    }
}

