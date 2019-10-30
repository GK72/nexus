// **********************************************
// ** gkpro @ 2019-10-27                       **
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

//std::string getKey(const Parser::record& rec, const std::string_view& sv)
glib::IO::RType getKey(const Parser::record& rec, const std::string_view& sv)
{
    return rec.at(std::string(sv));
    //return std::any_cast<std::string>(m_record.at(std::string(sv)));
    //return std::any_cast<std::string>(
    //    std::any_cast<Parser::record>(
    //        rec.at("travel")).at(
    //            std::string(sv)));
}

ParserJSON::container* ParserJSON::read()
{
    // TODO: remove m_data from the class members
    // Delete this function
    while ((m_record = readRecord()).size() > 0) {
        m_data->push_back(m_record);
    }
    return m_data;
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
        ++level;

        while (doRead) {
            ss << (ch = m_inf.get());
            if (ch == '{') {
                m_inf.unget();
                ss.get();
                //++level;
                std::string key_str = ss.str();
                std::string_view key = key_str;
                size_t start = key.find_last_of(",") + 1;
                ss.str(key_str.substr(0, start - 1));
                size_t end = key.find_last_of(':');
                if (end == std::string::npos) {
                    throw ParseErrorException();
                }
                key = strip(key.substr(start, end - start));
                //std::any value = readRecord();
                //rec[std::string(key)] = value;
                rec[std::string(key)] = readRecord();
            }
            else if (ch == '}') {
                if (--level == 0) {
                    doRead = false;
                    m_tokenizer->setString(ss.str());
                    readKeyValuePair(rec);
                }
            }
        }
    }

    return rec;
}

void ParserJSON::readKeyValuePair(record& rec)
{
    std::string key;
    std::any value;
    while ((key = std::string(readToken())).size() > 0) {
        //value = std::string(readToken());
        //rec[key] = value;
        rec[key] = std::string(readToken());
    }
}

RType RType::getKey(const std::string& key) const
{
    return asRecord().at(key);
}



} // End of namespace IO
} // End of namespace glib
