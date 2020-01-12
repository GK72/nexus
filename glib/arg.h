#pragma once
#include <any>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace glib {

using gint = size_t;

class ValuelessArgException : public std::runtime_error {
public:
    ValuelessArgException(const std::string& arg) : std::runtime_error("No value for argument: " + arg + '\n') {}
};

class ArgParsingException : public std::runtime_error {
public:
    ArgParsingException(const std::string& msg) : std::runtime_error("Parsing error: " + msg + '\n') {}
};

class InactiveArgException : public std::runtime_error {
public:
    InactiveArgException(const std::string& msg) : std::runtime_error("Inactive argument error: " + msg + '\n') {}
};

struct Arg {
    std::string name;
    std::string value;
    std::string description;
    bool isRequired = false;
    bool isOption = false;
    bool isActive = false;

    Arg::Arg(const std::string& name);
    Arg::Arg(const std::string& name, const std::string& description);
    Arg::Arg(const std::string& name, const std::string& description, const std::string& defaultValue);
    Arg::Arg(const std::string& name, const std::string& description, bool required, bool option);
    std::string_view getValue(const std::string& defaultValue = "") const;
};

class ArgParser {
public:
    ArgParser(int argc, char* argv[]);
    void add(const Arg& arg);
    Arg  get(const std::string_view& name);
    void process();

private:
    int m_argc;
    std::vector<std::string> m_inArgs;
    std::map<std::string, Arg> m_args;

    void argsToString(char* argv[]);
    void checkArgs();
    void displayHelp();
    void parseArgs();
    void setOptions();
};


} // End of namespace glib