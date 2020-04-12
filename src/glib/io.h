// **********************************************
// ** gkpro @ 2020-04-11                       **
// **                                          **
// **           ---  G-Library  ---            **
// **                IO header                 **
// **                                          **
// **********************************************

#pragma once
#include <any>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

#include "utility.h"


namespace glib::IO {

using ParseFile   = std::ifstream;
using ParseString = std::istringstream;

class RType;

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


class ParserCSV {
public:
    using Record = std::vector<RType>;

    ParserCSV(std::string_view path);
    ~ParserCSV();
    ParserCSV(const ParserCSV&)             = delete;
    ParserCSV(ParserCSV&&)                  = delete;
    ParserCSV& operator=(const ParserCSV&)  = delete;
    ParserCSV& operator=(ParserCSV&&)       = delete;

    std::map<std::string, size_t> readHeader();
    Record readRecord();
    std::string readToken();

private:
    std::ifstream m_inf;
    std::string m_path;
    Tokenizer* m_tokenizer;
    std::map<std::string, size_t> m_header;
    size_t m_length = 0;

};

template <class Input>
class ParserJSON {
public:
    using Record = std::map<std::string, std::any>;

    ParserJSON(const std::string& str);
    ~ParserJSON();
    ParserJSON(const ParserJSON&)               = delete;
    ParserJSON(ParserJSON&&)                    = delete;
    ParserJSON& operator=(const ParserJSON&)    = delete;
    ParserJSON& operator=(ParserJSON&&)         = delete;

    Record readRecord();

private:
    Input      m_input;
    Tokenizer* m_tokenizer;
    bool       m_quoted = false;

    bool doRead(char ch);
    std::pair<std::any, int> parseValue(const std::string& value);
    std::vector<std::any>    parseList(const std::string& value);
    std::string              readToken();
    std::string              strip(const std::string& str);

    void readKeyValuePair(Record& rec, Record& inner);

};

class RType {
public:
    enum class Type {
        EMPTY = 0
        ,RECORD
        ,LIST
        ,VALUE_STRING
        ,VALUE_INT
        ,VALUE_FLOAT
        ,VALUE_BOOL
    };

    RType() {}
    RType(const RType&) = default;
    // RType(const Parser::record& rec)                    { value = rec; type = Type::RECORD; }
    RType(const std::vector<RType>& vec)                { value = vec; type = Type::LIST; }
    RType(const std::string& str)                       { value = str; type = Type::VALUE_STRING; }
    RType(std::string_view sv)                          { value = sv ; type = Type::VALUE_STRING; }
    RType(int num)                                      { value = num; type = Type::VALUE_INT; }
    RType(float num)                                    { value = num; type = Type::VALUE_FLOAT; }
    RType(bool bln)                                     { value = bln; type = Type::VALUE_BOOL; }

    // RType& operator=(const Parser::record& rec)         { value = rec; type = Type::RECORD;         return *this; }
    RType& operator=(const std::vector<RType>& vec)     { value = vec; type = Type::LIST  ;         return *this; }
    RType& operator=(const std::string& str)            { value = str; type = Type::VALUE_STRING;   return *this; }
    RType& operator=(std::string_view sv)               { value = sv ; type = Type::VALUE_STRING;   return *this; }
    RType& operator=(int num)                           { value = num; type = Type::VALUE_INT;      return *this; }
    RType& operator=(float num)                         { value = num; type = Type::VALUE_FLOAT;    return *this; }
    RType& operator=(bool bln)                          { value = bln; type = Type::VALUE_BOOL;     return *this; }

    [[nodiscard]] std::string getTypeName()     const;
    [[nodiscard]] const auto getValue()         const   { return value; }
    // [[nodiscard]] const auto asRecord()         const   { return cast<Parser::record>(); }
    [[nodiscard]] const auto asList()           const   { return cast<std::vector<RType>>(); }
    [[nodiscard]] const auto asString()         const   { return cast<std::string>(); }
    [[nodiscard]] const auto asInt()            const   { return cast<int>(); }
    [[nodiscard]] const auto asFloat()          const   { return cast<float>(); }
    [[nodiscard]] const auto asBool()           const   { return cast<bool>(); }

    // [[nodiscard]]
    // const RType getKey(std::string_view key)    const   { return asRecord().at(key.data()); }

private:
    Type type = Type::EMPTY;
    std::any value;

    template <typename T> T cast() const {
        try {
            return std::any_cast<T>(value);
        }
        catch (const std::bad_any_cast& ex) {
            glib::dumpError(ex);
        }
        return T{};
    }

};



} // End of namespace glib::IO
