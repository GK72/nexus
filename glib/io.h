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

using gint = size_t;

class Tokenizer {
public:
    Tokenizer(const std::vector<std::string>& delims);
    const std::string_view& next();
    void setString(const std::string_view& sv);

private:
    std::string_view m_sv;
    std::vector<std::string> m_delims;

    gint m_posStart = 0;
    gint m_posEnd = 0;
    gint m_idxDelim = 0;
    gint m_nDelims;

    std::string_view m_str;
};


class Parser {
public:
    using record = std::map<std::string, std::any>;
    using container = std::vector<record>;

    Parser() {}
    virtual ~Parser() {}
    Parser(const Parser&)               = delete;
    Parser& operator=(const Parser&)    = delete;
    virtual container read()            = 0;
    virtual std::string readToken()     = 0;
};


class ParserJSON : public Parser {
public:
    ParserJSON(const std::string_view& path);
    container read() override;
    void nextRecord();
    std::string readToken() override;

private:
    std::ifstream m_inf;
    std::string m_path;
    std::stringstream m_ss;
    bool m_doRead = true;
    gint m_nRecords = 0;
    char m_ch;

    std::string mt_line;
    std::string mt_token;
    std::string m_key;
    std::any m_value;

    Tokenizer* m_tokenizer;
    record m_record;
    container m_data;

};

} // End of namespace glib