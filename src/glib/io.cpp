// **********************************************
// ** gkpro @ 2020-04-10                       **
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

Tokenizer::Tokenizer(const std::string& delim, const std::string& end)
{
    m_delims.push_back(delim);
    m_nDelims = 1;
    m_endMark = end;
}

Tokenizer::Tokenizer(const std::vector<std::string>& delims, const std::string& end)
    : m_delims(delims)
{
    m_nDelims = delims.size();
    m_endMark = end;
}

std::string Tokenizer::next()
{
    std::string token;
    if (m_isEnd) { return ""; }

    m_posEnd = m_str.find(m_delims[m_idxDelim], m_posStart);

    if (m_posEnd == std::string::npos) {
        if (m_endMark != "") {
            m_posEnd = m_str.find(m_endMark, m_posEnd + 1);
        }
        m_isEnd = true;
    }

    if (!m_quote.empty()) {
        escapeQuotes();
    }

    token = m_str.substr(m_posStart, m_posEnd - m_posStart);
    m_posStart = m_posEnd + 1;
    m_idxDelim = ++m_idxDelim % m_nDelims;

    return token;
}

void Tokenizer::clear()
{
    m_posStart = 0;
    m_posEnd   = 0;
    m_idxDelim = 0;
    m_str      = "";
    m_isEnd    = false;
}

void Tokenizer::escapeQuotes() {
    gint posQuote = m_str.find(m_quote, m_posStart);

    if (posQuote < m_posEnd) {    // There is a quote in the current token
        posQuote = m_str.find(m_quote, posQuote + 1);
        if (posQuote > m_posEnd) {      // There is a delim between the quotes
            m_posEnd = m_str.find(m_quote, posQuote);
            m_posEnd = m_str.find(m_delims[m_idxDelim], m_posEnd + 1);
        }
    }
}

ParserCSV::ParserCSV(std::string_view path)
{
    m_path = path;
    m_inf = std::ifstream(m_path);
    m_tokenizer = new Tokenizer(std::vector<std::string>{";"}, "\n");
}

ParserCSV::~ParserCSV()
{
    delete m_tokenizer;
}

ParserCSV::record ParserCSV::readRecord()
{
    std::string str;
    std::stringstream ss;
    record fields;

    if (getline(m_inf, str)) {
        ss.clear();
        ss.str(str);

        m_tokenizer->setString(str);
        for (gint i = 0; i < m_length; ++i) {
            fields.emplace_back(readToken());
        }
    }

    return fields;
}

std::string ParserCSV::readToken()
{
    return m_tokenizer->next();
}

std::map<std::string, gint> ParserCSV::readHeader()
{
    std::string str;
    std::stringstream ss;
    std::map<std::string, gint> header;

    if (getline(m_inf, str)) {
        ss.clear();
        ss.str(str);

        std::string value;
        gint count = 0;
        m_tokenizer->setString(str);
        while ((value = readToken()).size() > 0) {
            if (auto [it, success] = header.insert({ value, count }); !success) {
                header.insert({ value += "#", count });
            }
            ++count;
        }
    }

    m_length = header.size();
    return header;
}

ParserJSON::ParserJSON(std::string_view path)
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

std::string RType::getTypeName() const
{
    switch (type)
    {
    case glib::IO::RType::Type::EMPTY:              return "Empty";         break;
    case glib::IO::RType::Type::RECORD:             return "Record";        break;
    case glib::IO::RType::Type::LIST:               return "List";          break;
    case glib::IO::RType::Type::VALUE_STRING:       return "String";        break;
    case glib::IO::RType::Type::VALUE_INT:          return "Int";           break;
    default:                                        return "Unknown";       break;
    }
}



} // End of namespace IO
} // End of namespace glib