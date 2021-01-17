/*
 * gkpro @ 2020-05-23
 *   Sandbox for Nexus Library
 *   Template for using ArgParser
 */

#define NXSver 2020

#include <iostream>
#include <memory>

#include "arg.h"
#include "utility.h"
#include "ui.h"
#include "sandbox_measure.h"
#include "sandbox_pmr.h"

constexpr int UNKNOWN_ERROR = 255;

void runUI();

int run([[maybe_unused]] const nxs::ArgParser& args) {
    if (args.get<bool>("ui")) {
        runUI();
    }

    if (args.get<bool>("measure")) {
        nxs::sandbox::run();
    }

    if (args.get<bool>("pmr")) {
        nxs::sandbox::pmr();
    }

    return 0;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
nxs::ArgParser argParsing(int argc, char* argv[]) {
    nxs::ArgParser args(argc, argv);
    nxs::ArgFactory factory(&args);
    factory.add("path", "Path to something");
    factory.addFlag("debug", "Debug flag");
    factory.addFlag("ui", "UI sandbox");
    factory.addFlag("measure", "Measuring");
    factory.addFlag("pmr", "PMR");
    return args;
}

int main(int argc, char* argv[]) {
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
        return UNKNOWN_ERROR;
    }
}


void runUI() {
    nxs::UI();
}
