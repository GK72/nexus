/*
 * gkpro @ 2020-05-23
 *   Sandbox for Nexus Library
 *   Template for using ArgParser
 */

#include <memory>

#include "arg.h"
#include "utility.h"
#include "ui.h"

void grid(nxs::UI* ui) {
    auto x = nxs::UIInputGrid(ui, nxs::CoordsRC{}, {
        nxs::UIInput(ui, nxs::CoordsRC{}, "Input 1:", nxs::Format{}, "default value one"),
        nxs::UIInput(ui, nxs::CoordsRC{}, "Input 2:", nxs::Format{}, "default value two"),
        nxs::UIInput(ui, nxs::CoordsRC{}, "Input 3:", nxs::Format{}, "default value three")
    });

    ui->input(x);

    // nxs::UIMessage(ui, x.result(0)).execute();
}

int run(const nxs::ArgParser& args)
{
    // nxs::print("Empty sandbox");

    auto ui = nxs::UI();
    auto builder = nxs::MenuBuilder(ui);
    builder.add<nxs::UIMessage>("Help", "A message");
    builder.add("Grid test", [&ui]() { grid(&ui); });

    auto menu = builder.build();
    ui.input(menu);

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

