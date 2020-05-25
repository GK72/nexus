#include <chrono>
#include <functional>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "arg.h"
#include "datetime.h"
#include "utility.h"


static const auto pathDir       = nxs::joinStr("/", getenv("HOME"), ".worklog");
static const auto filename      = "data";
static const auto fullpathData  = nxs::joinStr("/", pathDir, filename);

constexpr char eof = 26;
constexpr auto rangeDivider = "-";

constexpr auto fn_getline = [](auto& inf, std::string& str) -> auto& { return std::getline(inf, str); };

enum class SummaryType {
    INVALID
    ,DAY
    ,WEEK
    ,MONTH
};

auto now() {
    return nxs::datetime::getCurrentEpoch<std::chrono::seconds>().count();
}

auto strSplit(std::string_view str, std::string_view split) {
    std::vector<std::string_view> parts;
    size_t start = 0;
    size_t end = 0;

    while (end = str.find(split, end), end != std::string_view::npos) {
        parts.push_back(str.substr(start, end));
        start = ++end;
    }
        parts.push_back(str.substr(start));

    return parts;
}

SummaryType validateSummaryType(std::string_view type) {
    if (type == "day") {
        return SummaryType::DAY;
    }
    else if (type == "week") {
        return SummaryType::WEEK;
    }
    else if (type == "month") {
        return SummaryType::MONTH;
    }
    else {
        return SummaryType::INVALID;
    }
}

class FileReader {
public:
    explicit FileReader(const std::string& path);
    FileReader(const FileReader&)               = delete;
    FileReader(FileReader&&)                    = delete;
    FileReader& operator=(const FileReader&)    = delete;
    FileReader& operator=(FileReader&&)         = delete;
    ~FileReader() { _inf.close(); }

    template <class Tokenizer, class InputProcessor>
    bool read(Tokenizer tokenizer, InputProcessor process);

private:
    std::string     _path;
    std::ifstream   _inf;
};

FileReader::FileReader(const std::string& path)
    : _path(path)
{
    _inf.open(_path);
}

template <class Tokenizer, class InputProcessor>
bool FileReader::read(Tokenizer tokenizer, InputProcessor process)
{
    if (!_inf) { return false; }
    std::string str;
    while (tokenizer(_inf, str)) {
        process(str);
    }
    return true;
}


class WorkLog {
    using Date = nxs::datetime::Date<>;
public:
    explicit WorkLog(const std::string& path);

    void checkIn();
    void checkOut();
    void createEntries();
    void printSummary(SummaryType type);

private:
    enum class State {
        CANNOT_OPEN
        ,EMPTY
        ,ONGOING
        ,CLOSED
        ,CORRUPTED
    };

    std::vector<Date>   _keys;
    std::vector<long>   _durations;
    std::string         _path;
    State               _state;

    State read();

};

WorkLog::WorkLog(const std::string& path)
    : _path(path)
    , _state(read())
{}

void WorkLog::checkIn() {
    if (_state == State::CLOSED) {
        std::ofstream outf(_path, std::ios::out | std::ios::in);
        if (outf) {
            outf.seekp(-1, std::ios::end);
            outf << now();
            outf.seekp(0, std::ios::end);
            outf << rangeDivider;
            outf.seekp(0, std::ios::end);
            outf << '\n';
        }
        outf.close();
    }
    else if (_state == State::ONGOING) {
        nxs::print("There is an ongoing logging.");
    }
    else {
        nxs::print("Error");
    }
}

void WorkLog::checkOut() {
    if (_state == State::ONGOING) {
        std::ofstream outf(_path, std::ios::out | std::ios::in);
        if (outf) {
            outf.seekp(-1, std::ios_base::end);
            outf << now();
            outf.seekp(0, std::ios_base::end);
            outf << "\n\n";
        }
        outf.close();
    }
    else if (_state != State::ONGOING) {
        nxs::print("There are no ongoing logging.");
    }
    else {
        nxs::print("Error.");
    }
}


void WorkLog::createEntries() {
    if (!(_state == State::ONGOING || _state == State::CLOSED)) {
        nxs::print("Invalid file!");
        return;
    }

    auto inf = FileReader(_path);

    inf.read(fn_getline,
        [this](const std::string& str) mutable {
            auto parts = strSplit(str, rangeDivider);
            long start = 0;
            long end   = 0;

            if (!parts[0].empty()) {
                start = std::stol(parts[0].data());

                if (!parts[1].empty()) {
                    end = std::stol(parts[1].data());
                }
                else {
                    end = now();
                }

                _keys.emplace_back(nxs::datetime::epochToDate(start));
                _durations.push_back(end - start);
            }
        }
    );
}

void WorkLog::printSummary(SummaryType type) {
    using namespace nxs::datetime;
    std::function<bool(Date, Date)> func;

    switch (type) {
        case SummaryType::DAY:
            func = [](Date key, Date prevKey) -> bool {
                return key == prevKey;
            };
            break;
        case SummaryType::WEEK:
            // weekNum() is not yet implemented
            // func = [](Date key, Date prevKey) -> bool {
            //     return weekNum(key) == weekNum(prevKey);
            // };
            break;
        case SummaryType::MONTH:
            func = [](Date key, Date prevKey) -> bool {
                return key.month() == prevKey.month();
            };
            break;
        default:    break;
    }

    auto summary = nxs::groupBy(_keys, _durations, func, nxs::AggregatePlusEquals);

    for (const auto& [date, duration] : summary) {
        nxs::print(
            nxs::joinStr(": "
                ,date.toString()
                ,nxs::datetime::prettyTime(duration)
            )
        );
    }
}

WorkLog::State WorkLog::read() {
    auto inf = FileReader(_path);
    auto state = State::CORRUPTED;

    inf.read(
        [](auto& inf, std::string& str) -> auto& { return std::getline(inf, str, eof); },
        [&state](auto& str)
        {
            if (*++str.rbegin() == '-') {
                state = State::ONGOING;
            }
            else if (*++str.rbegin() == '\n') {
                state = State::CLOSED;
            }
        }
    );

    return state;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
nxs::ArgParser parseArgs(int argc, char* args[]) {
    auto argparser = nxs::ArgParser(argc, args);
    auto argfactory = nxs::ArgFactory(&argparser);
    argfactory.addFlag("start", "Check-in");
    argfactory.addFlag("end", "Check-out");
    argfactory.addFlag("show");
    argfactory.add("summary");

    return argparser;
}


int main(int argc, char* argv[]) {
    auto args = parseArgs(argc, argv);

    try { args.process(); }
    catch (const std::runtime_error& e) {
        nxs::print(e.what());
        return 1;
    }

    if (args.get<bool>("--help")) { return 0; }

    auto log = WorkLog(fullpathData);

    try {
        if (args.get<bool>("start")) {
            log.checkIn();
        }
        else if (args.get<bool>("end")) {
            log.checkOut();
        }
        else if (args.get<bool>("show")) {
            auto type = validateSummaryType(args.get<std::string>("summary"));
            if (type == SummaryType::INVALID) {
                return 2;
            }
            log.createEntries();
            log.printSummary(type);
        }
    }
    catch (const std::runtime_error & e) {
        nxs::print(e.what());
    }
}

