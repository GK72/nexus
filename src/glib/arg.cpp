// **********************************************
// ** gkpro @ 2020-01-29                       **
// **                                          **
// **           ---  G-Library  ---            **
// **         Arguments implementation         **
// **                                          **
// **********************************************

#include "arg.h"
#include "utility.h"

namespace glib {

ArgParser::ArgParser(int argc, char* argv[])
{
    m_argc = argc;
    argsToString(argv);
    m_args.insert({ "help", ArgFactory::createFlag("Help", "Displays help") });
}

void ArgParser::add(const Arg& arg)
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
    glib::printn("Help");
}

void ArgParser::process()
{
    parseArgs();
    checkArgs();
    setOptions();
    if (m_args.at("help").getValue<bool>()) {
        displayHelp();
    }
}

void ArgParser::parseArgs()
{
    for (gint i = 0; i < m_inArgs.size(); ++i) {
        try {
            Arg& arg = m_args.at(m_inArgs[i].substr(2));
            arg.isActive = true;
            if (!arg.isFlag) {
                ++i;
                if (i >= m_inArgs.size()) {
                    throw ex::ValuelessArgException(m_inArgs[i - 1]);
                }
                arg.value = m_inArgs[i];
            }
        }
        catch (std::out_of_range) {
            throw ex::ArgParsingException("'" + m_inArgs[i] + "' is invalid argument");
        }
    }
}

void ArgParser::checkArgs()
{
    for (const auto& e : m_args) {
        if (e.second.isRequired && !e.second.isActive) {
            throw ex::InactiveArgException("'" + e.second.name + "' argument is required");
        }
    }
}

void ArgParser::setOptions()
{
    for (auto& e : m_args) {
        if (e.second.isFlag) {
            if (e.second.isActive) {
                e.second.value = true;
            }
            else {
                e.second.isActive = true;
                e.second.value = false;
            }
        }
    }
}



} // End of namespace glib