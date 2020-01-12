#include <iostream>
#include "arg.h"

namespace glib {

Arg::Arg(const std::string& name) : name(name) {}

Arg::Arg(const std::string& name, const std::string& description)
    : name(name), description(description) {}

Arg::Arg(const std::string& name, const std::string& description, const std::string& defaultValue)
    : name(name), description(description), value(defaultValue), isActive(true) {}

Arg::Arg(const std::string& name, const std::string& description, bool required, bool option)
    : name(name), description(description), isRequired(required), isOption(option) {}

std::string_view Arg::getValue(const std::string& defaultValue) const
{
    if (!isActive) {
        if (defaultValue == "") {
            throw InactiveArgException(name);
        }
        return defaultValue;
    }
    return value;
}

ArgParser::ArgParser(int argc, char* argv[])
{
    m_argc = argc;
    argsToString(argv);
    m_args.insert({ "help", Arg("Help", "Displays help", false, true) });
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
    std::cout << "Help\n";
}

Arg ArgParser::get(const std::string_view& name)
{
    return m_args.at(name.data());
}

void ArgParser::process()
{
    parseArgs();
    checkArgs();
    setOptions();
    if (m_args.at("help").value == "true") {
        displayHelp();
    }
}

void ArgParser::parseArgs()
{
    for (gint i = 0; i < m_inArgs.size(); ++i) {
        try {
            Arg& arg = m_args.at(m_inArgs[i]);
            arg.isActive = true;
            if (!arg.isOption) {
                ++i;
                if (i >= m_inArgs.size()) {
                    throw ValuelessArgException(m_inArgs[i - 1]);
                }
                arg.value = m_inArgs[i];
            }
        }
        catch (std::out_of_range) {
            throw ArgParsingException("'" + m_inArgs[i] + "' is invalid argument");
        }
    }
}

void ArgParser::checkArgs()
{
    for (const auto& e : m_args) {
        if (e.second.isRequired && !e.second.isActive) {
            throw InactiveArgException("'" + e.second.name + "' argument is required");
        }
    }
}

void ArgParser::setOptions()
{
    for (auto& e : m_args) {
        if (e.second.isOption) {
            if (e.second.isActive) {
                e.second.value = "true";
            }
            else {
                e.second.isActive = true;
                e.second.value = "false";
            }
        }
    }
}



} // End of namespace glib