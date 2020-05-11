// **********************************************
// ** gkpro @ 2020-04-11                       **
// **                                          **
// **           ---  G-Library  ---            **
// **            IO implementation             **
// **                                          **
// **********************************************

#include <cstdlib>

#include "io.h"
#include "utility.h"

namespace glib::IO {

using namespace std::literals::string_literals;

std::string_view trim(std::string_view sv, const std::string& what)
{
    size_t posStart = sv.find_first_not_of(what);
    size_t posEnd = sv.find_last_not_of(what);
    if (posEnd == std::string::npos) {
        return sv;
    }
    return sv = sv.substr(posStart, posEnd + 1 - posStart);
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
    std::string escapedToken = "";
    if (m_isEnd || m_str.empty()) {
        return "";
    }

    // TODO: make this compile time conditional i.e. static inheritance
    //       or make this default behaviour
    if (m_ignoreWhitespace) {
        while (m_posStart < m_str.size() && std::isspace(m_str.at(m_posStart))) {
            ++m_posStart;
        };
    }

    if (m_posStart >= m_str.size()) {
        return "";
    }

    auto esc = std::find_if(
        std::begin(m_escapers),
        std::end(m_escapers),
        [this](const auto& x) { return x.first.at(0) == m_str.at(m_posStart); }
    );

    if (esc != std::end(m_escapers)) {
        escapedToken = next(esc->second);
    }

    m_posEnd = m_str.find(m_delims[m_idxDelim], m_posStart);

    if (m_posEnd == std::string::npos) {
        if (m_endMark != "") {
            m_posEnd = m_str.find(m_endMark, m_posEnd + 1);
        }
        m_isEnd = true;
    }

    if (!m_quote.empty()) {
        escapeQuotes(m_delims[m_idxDelim]);
    }

    token = m_str.substr(m_posStart, m_posEnd - m_posStart);
    m_posStart = m_posEnd + 1;
    m_idxDelim = ++m_idxDelim % m_nDelims;

    if (escapedToken != "") {
        return escapedToken;
    }
    return token;
}

std::string Tokenizer::next(std::string_view sentinel)
{
    std::string token;
    if (m_isEnd) { return ""; }

    m_posEnd = m_str.find(sentinel, m_posStart);

    if (m_posEnd == std::string::npos) {
        if (m_endMark != "") {
            m_posEnd = m_str.find(m_endMark, m_posEnd + 1);
        }
        m_isEnd = true;
    }

    if (!m_quote.empty()) {
        escapeQuotes(sentinel);
    }

    token = m_str.substr(m_posStart, m_posEnd - m_posStart);
    m_posStart = m_posEnd + 1;

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

void Tokenizer::escapeQuotes(std::string_view quoteEnd) {
    size_t posQuote = m_str.find(m_quote, m_posStart);

    if (posQuote < m_posEnd) {    // There is a quote in the current token
        posQuote = m_str.find(m_quote, posQuote + 1);
        if (posQuote > m_posEnd) {      // There is a delim between the quotes
            m_posEnd = m_str.find(m_quote, posQuote);
            m_posEnd = m_str.find(quoteEnd, m_posEnd + 1);
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

ParserCSV::Record ParserCSV::readRecord()
{
    std::string str;
    std::stringstream ss;
    Record fields;

    if (getline(m_inf, str)) {
        ss.clear();
        ss.str(str);

        m_tokenizer->setString(str);
        for (size_t i = 0; i < m_length; ++i) {
            fields.emplace_back(readToken());
        }
    }

    return fields;
}

std::string ParserCSV::readToken()
{
    return m_tokenizer->next();
}

std::map<std::string, size_t> ParserCSV::readHeader()
{
    std::string str;
    std::stringstream ss;
    std::map<std::string, size_t> header;

    if (getline(m_inf, str)) {
        ss.clear();
        ss.str(str);

        std::string value;
        size_t count = 0;
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

template <class Input>
ParserJSON<Input>::ParserJSON(const std::string& str)
    : m_input()
{
    m_input = Input(str);
    m_tokenizer = new Tokenizer(std::vector<std::string>{":", ","}, "}");
    m_tokenizer->setEscapers({ {{"["s}, {"]"s} }});
    m_tokenizer->setIgnoreWhitespace(true);
}

template <class Input>
ParserJSON<Input>::~ParserJSON()
{
    delete m_tokenizer;
}

template <class Input>
std::map<std::string, std::any> ParserJSON<Input>::readRecord()
{
    Record record;
    Record inner;
    if (m_input.eof()) {
        return record;
    }

    std::stringstream ss;

    for (char ch; (ch = m_input.get()), doRead(ch);) {
        if (ch == '\"') {
            m_quoted = !m_quoted;
        }
        if (ch == '{' && !m_quoted) {
            inner = readRecord();
        }
        ss << ch;
    }

    m_tokenizer->setString(ss.str());
    readKeyValuePair(record, inner);

    return record;
}

template <class Input>
inline bool ParserJSON<Input>::doRead(char ch)
{
    return m_input.good()
        && (m_quoted || ch != '}');
}

template <class Input>
void ParserJSON<Input>::readKeyValuePair(Record& rec, Record& inner)
{
    std::string key;
    while ((key = std::string(trim(readToken(), "\""))).size() > 0 && key != "{") {
        std::string value = readToken();
        if (!value.empty()) {
            auto [any, code] = parseValue(value);
            if (code == 1) {    // Code 1: record
                rec[key] = std::move(inner);
            }
            else {
                rec[key] = any;
            }
        }
    }

    if (key == "{") {
        rec = std::move(inner);
    }
}

template <class Input>
std::vector<std::any> ParserJSON<Input>::parseList(const std::string& value)
{
    auto tok = Tokenizer(",", "]");
    tok.setString(value.substr(1));

    std::vector<std::any> vec;

    for (auto str = tok.next(); str != ""; str = tok.next()) {
        vec.push_back(parseValue(strip(str)).first);
    }

    return vec;
}

template <class Input>
std::pair<std::any, int> ParserJSON<Input>::parseValue(const std::string& value)
{
    if (value.at(0) == '\"') {
        return { std::any{std::string(trim(value, "\""))}, 0};
    }
    if (value.at(0) == '{') {
        return { std::any{}, 1};
    }
    if (value.at(0) == '[') {
        return { std::any{ parseList(value) }, 2};
    }
    if (value == "true") {
        return { std::any{true}, 0};
    }
    if (value == "false") {
        return { std::any{false}, 0};
    }
    if (value.find(".") != std::string::npos) {
        return { std::any{std::stod(value)}, 0};
    }
    return { std::any{std::stoi(value)}, 0};
}

template <class Input>
std::string ParserJSON<Input>::readToken()
{
    return strip(m_tokenizer->next());
}

template <class Input>
std::string ParserJSON<Input>::strip(const std::string& str)
{
    return std::string(trim(str, std::array<std::string, 2>{ "\n", " " }));
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


template class ParserJSON<ParseFile>;
template class ParserJSON<ParseString>;



} // End of namespace glib::IO
