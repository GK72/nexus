// **********************************************
// ** gkpro @ 2019-10-30                       **
// **                                          **
// **           ---  G-Library  ---            **
// **            IO implementation             **
// **                                          **
// **********************************************

#include "io.h"
#include "utility.h"

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
}

ParserJSON::~ParserJSON()
{
    delete m_tokenizer;
}

ParserJSON::record ParserJSON::readRecord()
{
    record rec;
    
    if (!m_inf.eof()) {
        std::stringstream ss;
        gint level = 0;
        char ch;
        bool doRead = true;

        while ((ch = m_inf.get()) != '{') {
            if (!m_inf.good()) {
                doRead = false;
                break;
            }
        }

        while (doRead) {
            ss << (ch = m_inf.get());
            if (ch == '{') {
                m_inf.unget();
                ss.get();

                std::string key = ss.str();
                size_t start = key.find_last_of(",") + 1;
                ss.str("");
                ss << key.substr(0, start - 1);     // Because ss.str() resets the position

                size_t end = key.find_last_of(':');
                if (end == std::string::npos) {
                    throw ParseErrorException();
                }
                key = strip(key.substr(start, end - start));
                rec[std::string(key)] = readRecord();
            }
            else if (ch == '}') {
                doRead = false;
                m_tokenizer->setString(ss.str());
                readKeyValuePair(rec);
            }
        }
    }

    return rec;
}

void ParserJSON::readKeyValuePair(record& rec)
{
    std::string key;
    while ((key = std::string(readToken())).size() > 0) {
        rec[key] = std::string(readToken());
    }
}



} // End of namespace IO
} // End of namespace glib