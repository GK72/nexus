#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include "../glib/datetime.h"
#include "../glib/utility.h"

//#define GTEST(testname)

class Tester {
public:
    Tester(const std::string& testname) : m_testname(testname) {}

    void add(const std::string& testcasename, const std::function<bool()>& func) {
        m_testcases.push_back({testcasename, func});
    }

    int run() {
        bool failed = false;
        for (const auto& [name, func] : m_testcases) {
            if (func()) {
                printPass(name);
            }
            else {
                printFail(name);
                failed = true;
            }
        }
        return failed;
    }

private:
    std::vector<std::pair<std::string, std::function<bool()>>> m_testcases;
    std::string m_testname;
    bool m_success = true;

    void printFail(const std::string& msg) { glib::print(" ", "FAILED", msg); }
    void printPass(const std::string& msg) { glib::print(" ", "PASSED", msg); }
};



int main() {
    Tester tester("DateTime Test");

    tester.add("2000.01.01", []{
        auto date = glib::date::Date(2000,1,1);
        return date.isValid();
    });

    tester.add("2000.01.31", []{
        auto date = glib::date::Date(2000,1,31);
        return date.isValid();
    });

    tester.add("2000.02.01", []{
        auto date = glib::date::Date(2000,2,1);
        return date.isValid();
    });

    tester.add("2001.02.28", []{
        auto date = glib::date::Date(2000,2,28);
        return date.isValid();
    });

    tester.add("2000.02.29", []{
        auto date = glib::date::Date(2000,2,29);
        return date.isValid();
    });

    tester.add("2001.02.29 invalid", []{
        auto date = glib::date::Date(2001,2,29);
        return !date.isValid();
    });

    tester.add("2000.03.31", []{
        auto date = glib::date::Date(2000,3,31);
        return date.isValid();
    });

    tester.add("2000.04.31 invalid", []{
        auto date = glib::date::Date(2000,4,31);
        return !date.isValid();
    });

    tester.add("2000.05.31", []{
        auto date = glib::date::Date(2000,5,31);
        return date.isValid();
    });
    
    tester.add("2000.06.31 invalid", []{
        auto date = glib::date::Date(2000,6,31);
        return !date.isValid();
    });
    
    tester.add("2000.07.31", []{
        auto date = glib::date::Date(2000,7,31);
        return date.isValid();
    });
    
    tester.add("2000.08.31", []{
        auto date = glib::date::Date(2000,8,31);
        return date.isValid();
    });

    tester.add("2000.09.31 invalid", []{
        auto date = glib::date::Date(2000,9,31);
        return !date.isValid();
    });

    tester.add("2000.10.31", []{
        auto date = glib::date::Date(2000,10,31);
        return date.isValid();
    });

    tester.add("2000.11.31 invalid", []{
        auto date = glib::date::Date(2000,11,31);
        return !date.isValid();
    });

    tester.add("2000.12.31", []{
        auto date = glib::date::Date(2000,12,31);
        return date.isValid();
    });

    return tester.run();
}