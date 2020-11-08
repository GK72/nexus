/*
 * gkpro @ 2020-10-07
 *   Nexus Library
 *   Argument parsing header
 */

#pragma once

#include <algorithm>
#include <variant>

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

#ifndef NXSver
inline
#endif

namespace latest {

using T_Arg= std::variant<bool, std::string, int>;

class ArgumentNotFound : public std::runtime_error {
public:
    ArgumentNotFound(std::string_view msg) : std::runtime_error(msg.data()) {}
};

class InvalidArgument : public std::runtime_error {
public:
    InvalidArgument(std::string_view msg) : std::runtime_error(msg.data()) {}
};

struct Arg {
    std::string name;
    std::string argname;
    std::string desc;
    // T_Arg value;
    std::string value;
    bool flag = false;
    bool required = false;
    bool active = false;
};

class ArgParser {
public:
    // template <T> void add(const std::string& name, const std::string& argname, const std::string& desc, bool required = false);
    void add(const std::string& name, const std::string& argname, const std::string& desc, bool flag = false, bool required = false);
    void process(int argc, const char* args[]);

    auto get(std::string_view name);

private:
    std::vector<Arg> m_args;

    auto& find(std::string_view name) {
        auto it = std::find_if(std::begin(m_args), std::end(m_args), [name](const Arg& arg) { return arg.name == name; });
        if (it == std::end(m_args)) {
            throw ArgumentNotFound(name);
        }
        return *it;
    }

    void setActive(std::string_view name) { find(name).active = true; }
};

inline auto ArgParser::get(std::string_view name) { return find(name).value; }

// template <T>
inline void ArgParser::add(const std::string& name, const std::string& argname, const std::string& desc, bool flag, bool required) {
    // m_args.push_back(Arg<T>{ name, desc, T{}, required });
    m_args.push_back(Arg{ name, argname, desc, "", flag, required });
}

inline void ArgParser::process(int argc, const char* args[]) {
    for (size_t i = 1; i < argc; ++i) {
        auto it = std::find_if(
            std::begin(m_args),
            std::end(m_args),
            [&](const Arg& arg) { return arg.argname == std::string(args[i]); }
        );

        if (it == std::end(m_args)) {
            throw InvalidArgument(std::string_view(args[i]));
        }

        auto& arg = *it;

        arg.active = true;
        if (arg.flag) {
            arg.value = "true";
        }
        else {
            arg.value = std::string(args[++i]);
        }
    }
}

} // namespace latest


#if NXSver == 2020
inline
#endif

namespace V2020 {

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

} // namespace V2020

} // namespace nxs
