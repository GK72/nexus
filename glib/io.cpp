// **********************************************
// ** gkpro @ 2019-10-27                       **
// **                                          **
// **           ---  G-Library  ---            **
// **            IO implementation             **
// **                                          **
// **********************************************

#include "io.h"


namespace glib {
namespace IO {


std::string_view trim(std::string_view sv, const std::string& what)
{
    size_t posStart = sv.find_first_not_of(what);
    size_t posEnd = sv.find_last_not_of(what);
    if (posEnd == std::string::npos) {
        return sv;
    }
    return sv = sv.substr(posStart, posEnd + 1 - posStart);
}

std::string_view strip(std::string_view sv, std::vector<std::string>&& vec)
{
    for (const auto& v : vec) {
        sv = trim(sv, v);
    }
    return sv;
}

Tokenizer::Tokenizer(const std::vector<std::string>& delims, const std::string_view& end)
    : m_delims(delims)
{
    m_nDelims = delims.size();
    m_endMark = end;
}

std::string_view Tokenizer::next()
{
    // TODO: more general tokenizer; keep track of structure depth level
    // like JSON's recursive records
    if (!m_isEnd) {
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
    m_data = new container();
}

ParserJSON::~ParserJSON()
{
    delete m_data;
    delete m_tokenizer;
}

ParserJSON::container* ParserJSON::read()
{
    while ((m_record = readRecord()).size() > 0) {
        m_data->push_back(m_record);
    }
    return m_data;
}

ParserJSON::record ParserJSON::readRecord()
{
    // Sample
    // {"key": "value", ..., "key": {"key": ... } }
    m_record.clear();
    //m_tokenizer->clear();

    if (!m_inf.eof()) {
        std::stringstream ss;
        char ch;
        gint level = 0;
        bool doRead = true;

        // TODO: tokenize also records
        // Finding the start of the record
        while ((ch = m_inf.get()) != '{') {
            if (!m_inf.good()) {
                doRead = false;
                break;
            }
        }
        ++level;

        while (doRead) {
            ss << (ch = m_inf.get());
            std::cout << ch;
            if (ch == '{') {
                ++level;
                // Stopping in between a key and value (value is the new record)
                gint end = ss.str().find_last_of(",");
                std::string str = ss.str().substr(0, end);
                // Store key value pairs in a string
                //m_tokenizer->setString(str);
                //readKeyValuePair();

                str = ss.str().substr(end + 1);
                m_tokenizer->setString(str);
                std::string key;
                key = readToken();
                m_inf.putback(ch);
                --level;
                std::any value = readRecord();
                m_record.clear();
                m_record[key] = value;
            }
            else if (ch == '}') {
                if (--level == 0) { doRead = false; }
                m_tokenizer->setString(ss.str());
                readKeyValuePair();
            }
        }
        //m_tokenizer->setString(ss.str());
        //readKeyValuePair();
    }
    return m_record;
}

void ParserJSON::readKeyValuePair()
{
    std::string key;
    std::any value;
    while ((key = readToken()).size() > 0) {
        value = readToken();
        m_record[key] = value;
    }
}



} // End of namespace IO
} // End of namespace glib
