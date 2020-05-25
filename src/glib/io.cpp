/*
 * gkpro @ 2020-04-11
 *   G-Library
 *   IO implementation
 */

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


} // namespace glib::IO
