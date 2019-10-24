#include "io.h"


namespace glib {


Tokenizer::Tokenizer(const std::vector<std::string>& delims)
    : m_delims(delims)
{
    m_nDelims = delims.size();
}

const std::string_view& Tokenizer::next()
{
    m_posEnd = m_sv.find(m_delims[m_idxDelim], m_posEnd);
    m_str = m_posEnd != std::string_view::npos
        ? m_sv.substr(m_posStart, m_posEnd - m_posStart)
        : "";
    m_posStart = ++m_posEnd;
    m_idxDelim = ++m_idxDelim % m_nDelims;
    return m_str;
}

void Tokenizer::setString(const std::string_view& sv)
{
    m_sv = sv;
}


ParserJSON::ParserJSON(const std::string_view& path)
{
    m_path = path;
    m_inf = std::ifstream(m_path);
    m_tokenizer = new Tokenizer(std::vector<std::string>{":", ","});
}

Parser::container ParserJSON::read()
{
    while (!m_inf.eof()) {
        nextRecord();
        while ((m_key = readToken()).size() > 0) {
            m_value = readToken();
            m_record[m_key] = m_value;
        }
        m_data.push_back(m_record);
        m_doRead = true;
    }
    return m_data;
}

void ParserJSON::nextRecord()
{
    while (m_doRead) {
        m_ss << (m_ch = m_inf.get());
        if (m_ch == '{') { ++m_nRecords; }
        else if (m_ch == '}') { --m_nRecords; }
        if (m_nRecords == 0) {
            m_doRead = false;
        }
        mt_line = m_ss.str();
    }
}

std::string ParserJSON::readToken()
{
    //while (m_doRead) {
    //    m_ss << (m_ch = m_inf.get());
    //    if (m_ch == '{')        { ++m_nRecords; }
    //    else if (m_ch == '}')   { --m_nRecords; }
    //    if (m_nRecords == 0) {
    //        m_doRead = false;
    //    }
    //}

    m_tokenizer->setString(mt_line);
    mt_token = m_tokenizer->next();
    return mt_token;
}

} // End of namespace glib