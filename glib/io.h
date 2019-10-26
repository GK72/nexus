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

std::string trim(const std::string& str, const std::string& what);

class Tokenizer {
public:
    Tokenizer(const std::vector<std::string>& delims);
    const std::string& next();
    void setString(const std::string&& str);
    void clear();

private:
    std::string_view m_sv;
    std::vector<std::string> m_delims;

    gint m_posStart = 0;
    gint m_posEnd = 0;
    gint m_idxDelim = 0;
    gint m_nDelims;

    std::string m_str;
    std::string m_token;
};


class Parser {
public:
    using record = std::map<std::string, std::string>;
    using container = std::vector<record>;

    Parser() {}
    virtual ~Parser()                   = default;
    Parser(const Parser&)               = delete;
    Parser(Parser&&)                    = delete;
    Parser& operator=(const Parser&)    = delete;
    Parser& operator=(Parser&&)         = delete;

    virtual container read() = 0;
    virtual record readRecord() = 0;
    virtual const std::string& readToken() = 0;
};


class ParserJSON : public Parser {
public:
    ParserJSON(const std::string_view& path);
    container read() override { return container(); };
    record readRecord();
    const std::string& readToken() override;

private:
    std::ifstream m_inf;
    std::string m_path;
    Tokenizer* m_tokenizer;
    record m_record;
    container m_data;

    void readKeyValuePair();

};



} // End of namespace IO
} // End of namespace glib