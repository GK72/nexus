/*
 * gkpro @ 2020-01-19
 *   Nexus Library
 *   Argument parsing header
 */

#pragma once

#include <any>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace nxs::ex {

class ValuelessArgException : public std::runtime_error {
public:
    ValuelessArgException(const std::string& arg)
        : std::runtime_error("No value for argument: " + arg + '\n') {}
};

class ArgParsingException : public std::runtime_error {
public:
    ArgParsingException(const std::string& msg)
        : std::runtime_error("Parsing error: " + msg + '\n') {}
};

class InactiveArgException : public std::runtime_error {
public:
    InactiveArgException(const std::string& msg)
        : std::runtime_error("Inactive argument error: " + msg + '\n') {}
};


} // namespace nxs::ex

namespace nxs {


struct Arg {
    std::string name;
    std::any value;
    std::string description;
    bool isRequired = false;
    bool isFlag = false;
    bool isActive = false;

    Arg(const std::string& name, const std::string& description)
        : name(name), description(description) {}

    template <class T = std::string>
    T getValue(std::optional<T> defaultValue = {}) const {
        if (isActive) {
            return std::any_cast<T>(value);
        }
        else if (defaultValue.has_value()) {
            return defaultValue.value();
        }
        throw ex::InactiveArgException(name);
    }
};

class ArgParser {
public:
    ArgParser(int argc, char* argv[]);
    void add(const Arg& arg);
    void process();

    template <class T = std::string>
    T get(const std::string& name, std::optional<T> defaultValue = {}) const {
        return m_args.at(name).getValue<T>(defaultValue);
    }

    [[nodiscard]] std::string getExeName() const        { return m_exeName; }

private:
    int m_argc;
    std::vector<std::string> m_inArgs;
    std::map<std::string, Arg> m_args;
    std::string m_exeName;

    void argsToString(char* argv[]);
    void checkArgs();
    void displayHelp();
    void parseArgs();
    void setOptions();
};

class ArgFactory {
public:
    static Arg create(const std::string& name, const std::string& description = "") {
        return Arg(name, description);
    }
    static Arg createFlag(const std::string& name, const std::string& description = "") {
        Arg arg(name, description);
        arg.isFlag = true;
        return arg;
    }
    static Arg createMandatory(const std::string& name, const std::string& description = "") {
        Arg arg(name, description);
        arg.isRequired = true;
        return arg;
    }

    ArgFactory(ArgParser* parser) : m_parser(parser) {}

    void add(const std::string& name, const std::string& description = "") {
        m_parser->add(Arg(name, description));
    }
    void addFlag(const std::string& name, const std::string& description = "") {
        Arg arg(name, description);
        arg.isFlag = true;
        m_parser->add(arg);
    }
    Arg addMandatory(const std::string& name, const std::string& description = "") {
        Arg arg(name, description);
        arg.isRequired = true;
        m_parser->add(arg);

        return arg;
    }

private:
    ArgParser* m_parser;
};


} // namespace nxs
