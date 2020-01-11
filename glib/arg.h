#pragma once
#include <any>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace glib {

using gint = size_t;

struct Arg {
    std::string name;
    std::string value;
    std::string description;
    bool required = false;
    bool flag = false;
    bool active = false;

    Arg::Arg(std::string name, std::string description);
    Arg::Arg(std::string name, std::string description, bool required, bool flag);
};

class ArgParser {
public:
    ArgParser(int argc, char* argv[]);
    void add(Arg arg);
    Arg get(const std::string_view& name);
    void process();

private:
    int m_argc;
    std::vector<std::string> m_inArgs;
    std::map<std::string, Arg> m_args;

    void argsToString(char* argv[]);
    void displayHelp();
};


} // End of namespace glib