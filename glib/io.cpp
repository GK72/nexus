#include "io.h"


namespace glib {
namespace IO {


std::string& trim(std::string& str, const std::string& what)
{
    size_t posStart = str.find_first_not_of(what);
    size_t posEnd = str.find_last_not_of(what);
    if (posEnd == std::string::npos) {
        return str;
    }
    str = str.substr(posStart, posEnd + 1 - posStart);
    return str;
}

std::string& strip(std::string& str, std::vector<std::string>&& vec/* = { "\n", " ", "\"" }*/)
{
    //std::string outstr = str;
    for (const auto& v : vec) {
        //outstr = trim(outstr, v);
        str = trim(str, v);
    }
    //return outstr;
    return str;
}

Tokenizer::Tokenizer(const std::vector<std::string>& delims, const std::string_view& end)
    : m_delims(delims)
{
    m_nDelims = delims.size();
    m_endMark = end;
}

std::string& Tokenizer::next()
{
    // TODO: more general tokenizer; keep track of structure depth level
    // like JSON's recursive records
    if (!m_isEnd) {
    //if (m_posEnd < m_str.size()) {
        if (m_isInQuotes) {
            m_posEnd = m_str.find(m_quote, m_posEnd + 1);
        }
        else {
            m_posEnd = m_str.find(m_delims[m_idxDelim], m_posEnd + 1);
        }
        if (m_posEnd == std::string::npos) {
            m_posEnd = m_str.find(m_endMark, m_posEnd + 1);
            m_isEnd = true;
        }
        m_token = m_str.substr(m_posStart, m_posEnd - m_posStart);
        m_posStart = m_posEnd + 1;
        m_idxDelim = ++m_idxDelim % m_nDelims;
    }
    else {
        m_token = "";
    }

    return m_token;
}

void Tokenizer::clear()
{
    m_posStart = 0;
    m_posEnd = 0;
    m_idxDelim = 0;
    m_str = "";
    m_token = "";
    m_isEnd = false;
}

ParserJSON::ParserJSON(const std::string_view& path)
{
    m_path = path;
    m_inf = std::ifstream(m_path);
    m_tokenizer = new Tokenizer(std::vector<std::string>{":", ","}, "}");
}

ParserJSON::record ParserJSON::readRecord()
{
    m_record.clear();
    m_tokenizer->clear();

    if (!m_inf.eof()) {
        std::stringstream ss;
        char ch;
        gint level = 0;

        // TODO: tokenize also records
        // Finding the start of the record
        while (((ch = m_inf.get()) != '{')
            && (m_inf.good()));
        if (ch == '{') { ++level; }

        bool doRead = true;
        while (doRead) {
            ss << (ch = m_inf.get());
            if (ch == '{')      { ++level; }
            else if (ch == '}') { --level; }
            if (level == 0)  { doRead = false; }
        }
        m_tokenizer->setString(ss.str());
        readKeyValuePair();
    }
    return m_record;
}

void ParserJSON::readKeyValuePair()
{
    std::string key;
    std::string value;
    while ((key = readToken()).size() > 0) {
        value = readToken();
        m_record[key] = value;
    }
}



} // End of namespace IO
} // End of namespace glib