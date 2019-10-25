#include "io.h"


namespace glib {
namespace IO {


std::string trim(const std::string& str, const std::string& what)
{
    size_t posStart = str.find_first_not_of(what);
    size_t posEnd = str.find_last_not_of(what);
    if (posEnd == std::string::npos) {
        return str;
    }
    return str.substr(posStart, posEnd + 1 - posStart);
}


Tokenizer::Tokenizer(const std::vector<std::string>& delims)
    : m_delims(delims)
{
    m_nDelims = delims.size();
}

std::string Tokenizer::next()
{
    if (m_posEnd < m_str.size()) {
        m_posEnd = m_str.find(m_delims[m_idxDelim], m_posEnd + 1);
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
}

void Tokenizer::setString(const std::string&& str)
{
    m_str = str;
}


ParserJSON::ParserJSON(const std::string_view& path)
{
    m_path = path;
    m_inf = std::ifstream(m_path);
    m_tokenizer = new Tokenizer(std::vector<std::string>{":", ","});
}

ParserJSON::record ParserJSON::readRecord()
{
    bool doRead = true;
    m_record.clear();
    m_tokenizer->clear();

    if (!m_inf.eof()) {
        std::stringstream ss;
        while (((m_ch = m_inf.get()) != '{')
            && (m_inf.good()));
        if (m_ch == '{') { ++m_nRecords; }
        while (doRead) {
            ss << (m_ch = m_inf.get());

            if (m_ch == '{') { ++m_nRecords; }
            else if (m_ch == '}') { --m_nRecords; }
            if (m_nRecords == 0) {
                doRead = false;
            }
        }
        m_tokenizer->setString(ss.str());

        while ((m_key = readToken()).size() > 0) {
            m_value = trim(trim(readToken(), " "), "\"");
            m_key = trim(trim(m_key, " "), "\"");
            m_record[m_key] = m_value;
        }
    }
    return m_record;
}

std::string ParserJSON::readToken()
{
    return m_tokenizer->next();
}



} // End of namespace IO
} // End of namespace glib