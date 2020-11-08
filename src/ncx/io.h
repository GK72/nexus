/*
 * gkpro @ 2020-10-07
 *   Nexus Library
 *   IO header
 */

#pragma once

#include <any>
#include <array>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "utility.h"


namespace nxs {

#ifndef NXSver
inline
#endif

namespace latest {

// ----------------------------------------==[ FILE ]==------------------------------------------ //

template <class Callable>
class File {
public:
    File(const std::string& path, Callable process);
    ~File();

    size_t read();

private:
    std::fstream m_file;
    std::string m_buf;
    Callable m_process;
};

template <class Callable>
File<Callable>::File(const std::string& path, Callable process)
    : m_file(path)
    , m_process(process)
{
    if (!m_file.is_open()) {
        std::cerr << "Could not open the file\n";
    }
}

template <class Callable>
File<Callable>::~File() {
    m_file.close();
}

template <class Callable>
size_t File<Callable>::read() {
    size_t s = 0;

    while (std::getline(m_file, m_buf)) {
        m_process(m_buf);
        ++s;
    }

    return s;
}


} // namespace trunk
} // namespace nxs


namespace nxs::IO {

std::string_view trim(std::string_view sv, const std::string& what);

template <size_t N>
std::string_view trim(std::string_view sv, std::array<std::string, N> arr) {
    for (const auto& v : arr) {
        sv = trim(sv, v);
    }
    return sv;
}


class ParseErrorException : public std::runtime_error {
public:
    ParseErrorException() : std::runtime_error("Parser Error") {}
};

/**
 * @brief Creates tokens from a string based on the given delim char(s)
 *
 * @todo Currently it does not work with multicharacter delim
 */
class Tokenizer {
public:
    using Escaper = std::pair<std::string, std::string>;

    Tokenizer(const std::string& delim, const std::string& end = "");
    Tokenizer(const std::vector<std::string>& delims, const std::string& end = "");

    std::string next();
    std::string next(std::string_view sentinel);

    void setEscapers(const std::vector<Escaper>& escs)      { m_escapers = escs; }
    void setIgnoreWhitespace(bool b)                        { m_ignoreWhitespace = b; }
    void setString(const std::string& str)                  { clear(); m_str = str; }
    void setQuote(const std::string& sv)                    { m_quote = sv; }
    void clear();

private:
    std::vector<std::string> m_delims;
    std::vector<Escaper>     m_escapers;

    std::string m_endMark;
    std::string m_str;
    std::string m_quote = "";

    size_t m_nDelims;
    size_t m_idxDelim = 0;
    size_t m_posStart = 0;
    size_t m_posEnd   = 0;

    bool m_isEnd            = false;
    bool m_ignoreWhitespace = false;

    void escapeQuotes(std::string_view quoteEnd);
};


} // namespace nxs::IO
