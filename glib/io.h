// **********************************************
// ** gkpro @ 2019-10-27                       **
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


namespace glib {
namespace IO {

using gint = size_t;

std::string_view& trim(std::string_view& sv, const std::string& what);
std::string_view& strip(std::string_view& sv, std::vector<std::string>&& vec = { "\n", " ", "\"" });

class Tokenizer {
public:
    Tokenizer(const std::vector<std::string>& delims, const std::string_view& end);
    std::string_view next();
    void setString(const std::string_view& str) { clear(); m_str = str; }
    void setQuote(const std::string_view& sv) { m_quote = sv; }
    void clear();

private:
    std::vector<std::string> m_delims;
    std::string_view m_endMark;

    std::string_view m_sv;
    std::string m_str;
    std::string m_token;
    gint m_posStart = 0;
    gint m_posEnd = 0;
    gint m_idxDelim = 0;
    gint m_nDelims;
    bool m_isInQuotes = false;
    bool m_isEnd = false;
    std::string m_quote = "\"";

};


class Parser {
public:
    //using record = std::map<std::string, std::string>;
    using record = std::map<std::string, std::any>;
    using container = std::vector<record>;

    Parser() {}
    virtual ~Parser()                   = default;
    Parser(const Parser&)               = delete;
    Parser(Parser&&)                    = delete;
    Parser& operator=(const Parser&)    = delete;
    Parser& operator=(Parser&&)         = delete;

    virtual std::string getKey(const std::string_view& sv) = 0;

    virtual container* read() = 0;
    virtual record readRecord() = 0;
    virtual std::string_view readToken() = 0;
};


class ParserJSON : public Parser {
public:
    ParserJSON(const std::string_view& path);
    ~ParserJSON();
    ParserJSON(const ParserJSON&)               = delete;
    ParserJSON(ParserJSON&&)                    = delete;
    ParserJSON& operator=(const ParserJSON&)    = delete;
    ParserJSON& operator=(ParserJSON&&)         = delete;

    std::string getKey(const std::string_view& sv);

    container* read() override;
    record readRecord();
    std::string_view readToken() override { return strip(m_tokenizer->next()); };

private:
    std::ifstream m_inf;
    std::string m_path;
    Tokenizer* m_tokenizer;
    record m_record;
    container* m_data;          // TODO: remove m_data

    void readKeyValuePair();

};



} // End of namespace IO
} // End of namespace glib