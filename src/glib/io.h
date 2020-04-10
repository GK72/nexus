// **********************************************
// ** gkpro @ 2020-04-10                       **
// **                                          **
// **           ---  G-Library  ---            **
// **                IO header                 **
// **                                          **
// **********************************************

#pragma once
#include <any>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

#include "utility.h"


namespace glib {
namespace IO {

class RType;

std::string_view trim(std::string_view sv, const std::string& what);
std::string_view strip(std::string_view sv, std::vector<std::string>&& vec = { "\n", " ", "\"" });

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
    Tokenizer(const std::string& delim, const std::string& end = "");
    Tokenizer(const std::vector<std::string>& delims, const std::string& end = "");

    std::string next();

    void setString(const std::string& str)      { clear(); m_str = str; }
    void setQuote(const std::string& sv)        { m_quote = sv; }
    void clear();

private:
    std::vector<std::string> m_delims;

    std::string m_endMark;
    std::string m_str;
    std::string m_quote = "";

    size_t m_nDelims;
    size_t m_idxDelim = 0;
    size_t m_posStart = 0;
    size_t m_posEnd   = 0;

    bool m_isEnd = false;

    void escapeQuotes();
};


class Parser {
public:
    using record = std::map<std::string, RType>;
    using container = std::vector<record>;

    Parser() {}
    virtual ~Parser()                   = default;
    Parser(const Parser&)               = delete;
    Parser(Parser&&)                    = delete;
    Parser& operator=(const Parser&)    = delete;
    Parser& operator=(Parser&&)         = delete;

    //virtual record readRecord() = 0;
    //virtual std::string_view readToken() = 0;
};


class ParserCSV : Parser {
public:
    using record = std::vector<RType>;

    ParserCSV(std::string_view path);
    ~ParserCSV();
    ParserCSV(const ParserCSV&)             = delete;
    ParserCSV(ParserCSV&&)                  = delete;
    ParserCSV& operator=(const ParserCSV&)  = delete;
    ParserCSV& operator=(ParserCSV&&)       = delete;

    std::map<std::string, size_t> readHeader();
    record readRecord();
    std::string readToken();

private:
    std::ifstream m_inf;
    std::string m_path;
    Tokenizer* m_tokenizer;
    std::map<std::string, size_t> m_header;
    size_t m_length = 0;

};

class ParserJSON : public Parser {
public:
    ParserJSON(std::string_view path);
    ~ParserJSON();
    ParserJSON(const ParserJSON&)               = delete;
    ParserJSON(ParserJSON&&)                    = delete;
    ParserJSON& operator=(const ParserJSON&)    = delete;
    ParserJSON& operator=(ParserJSON&&)         = delete;

    record readRecord();
    std::string_view readToken()               { return strip(m_tokenizer->next()); };

private:
    std::ifstream m_inf;
    std::string m_path;
    Tokenizer* m_tokenizer;

    void readKeyValuePair(record& rec);

};

class RType {
public:
    enum class Type {
        EMPTY = 0
        ,RECORD
        ,LIST
        ,VALUE_STRING
        ,VALUE_INT
    };

    RType() {}
    RType(const RType&) = default;
    RType(const Parser::record& rec)                    { value = rec; type = Type::RECORD; }
    RType(const std::vector<RType>& vec)                { value = vec; type = Type::LIST; }
    RType(const std::string& str)                       { value = str; type = Type::VALUE_STRING; }
    RType(std::string_view sv)                          { value = sv ; type = Type::VALUE_STRING; }
    RType(int num)                                      { value = num; type = Type::VALUE_INT; }

    RType& operator=(const Parser::record& rec)         { value = rec; type = Type::RECORD; return *this; }
    RType& operator=(const std::vector<RType>& vec)     { value = vec; type = Type::LIST  ; return *this; }
    RType& operator=(const std::string& str)            { value = str; type = Type::VALUE_STRING; return *this; }
    RType& operator=(std::string_view sv)               { value = sv ; type = Type::VALUE_STRING; return *this; }
    RType& operator=(int num)                           { value = num; type = Type::VALUE_INT; return *this; }

    [[nodiscard]] std::string getTypeName() const;
    [[nodiscard]] const auto getValue() const           { return value; }
    [[nodiscard]] const auto asRecord() const           { return cast<Parser::record>(); }
    [[nodiscard]] const auto asList() const             { return cast<std::vector<RType>>(); }
    [[nodiscard]] const auto asString() const           { return cast<std::string>(); }
    [[nodiscard]] const auto asInt() const              { return cast<int>(); }

    [[nodiscard]]
    const RType getKey(std::string_view key) const      { return asRecord().at(key.data()); }

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



} // End of namespace IO
} // End of namespace glib