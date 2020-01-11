#include <iostream>
#include "arg.h"

namespace glib {

Arg::Arg(std::string name, std::string description)
    : name(name), description(description) {}

Arg::Arg(std::string name, std::string description, bool required, bool flag)
    : name(name), description(description), required(required), flag(flag) {}

ArgParser::ArgParser(int argc, char* argv[])
{
    m_argc = argc;
    argsToString(argv);
    m_args.insert({ "help", Arg("Help", "Displays help", false, true) });
}

void ArgParser::add(Arg arg)
{
    m_args.insert({arg.name, arg});
}

void ArgParser::argsToString(char* argv[])
{
    for (gint i = 1; i < m_argc; ++i) {
        std::string str(argv[i]);
        m_inArgs.push_back(argv[i]);
    }
}

void ArgParser::displayHelp()
{
    std::cout << "Help\n";
}

Arg ArgParser::get(const std::string_view& name)
{
    return m_args.at(name.data());
}

void ArgParser::process()
{
    for (gint i = 0; i < m_inArgs.size(); ++i) {
        try {
            Arg& arg = m_args.at(m_inArgs[i]);
            if (arg.flag) {
                arg.active = true;
            }
            else {
                ++i;
                // TODO: check if value exists
                arg.value = m_inArgs[i];
            }
        }
        catch (std::out_of_range) {
            std::cout << "Invalid argument: " << m_inArgs[i] << "\nUse help for available arguments\n";
        }
    }
    if (m_args.at("help").active) {
        displayHelp();
        exit(0);
    }
}

} // End of namespace glib