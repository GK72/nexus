#include "catch.hpp"
#include "io.h"

using namespace std::literals::string_literals;

namespace glib::test {

TEST_CASE("Tokenizer - One delim", "[tokenizer]") {
    auto tok = glib::IO::Tokenizer(",");

    SECTION("Empty") {
        tok.setString("");
        CHECK(tok.next() == "");
    }

    SECTION("String without delim") {
        tok.setString("bla");
        CHECK(tok.next() == "bla");
    }

    SECTION("Continuing processing after the end") {
        tok.setString("bla");
        CHECK(tok.next() == "bla");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "");
    }

    SECTION("String with only delim") {
        tok.setString(",");
        CHECK(tok.next() == "");
    }

    SECTION("One part leading delim") {
        tok.setString(",bla1");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla1");
    }

    SECTION("One part trailing delim") {
        tok.setString("bla1,");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "");
    }

    SECTION("Two parts") {
        tok.setString("bla1,bla2");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "bla2");
    }

    SECTION("Three parts plus last but one empty") {
        tok.setString("bla1,bla2,,bla3");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "bla2");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla3");
    }
}

TEST_CASE("Tokenizer - One delim, multiple chars", "[tokenizer][!mayfail]") {
    auto tok = glib::IO::Tokenizer("./");

    SECTION("Empty") {
        tok.setString("");
        CHECK(tok.next() == "");
    }

    SECTION("String without delim") {
        tok.setString("bla");
        CHECK(tok.next() == "bla");
    }

    SECTION("String with only delim") {
        tok.setString("./");
        CHECK(tok.next() == "");
    }

    SECTION("String with only delim, twice") {
        tok.setString("././");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "");
    }

    SECTION("One part leading delim") {
        tok.setString("./bla1");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla1");
    }

    SECTION("One part trailing delim") {
        tok.setString("bla1./");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "");
    }

    SECTION("Two parts") {
        tok.setString("bla1./bla2");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "bla2");
    }

    SECTION("Three parts plus last but one empty") {
        tok.setString("bla1./bla2././bla3");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "bla2");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla3");
    }
}

TEST_CASE("Tokenizer - Two delims", "[tokenizer]") {
    auto tok = glib::IO::Tokenizer(std::vector{"{"s, "}"s});

    SECTION("Empty") {
        tok.setString("");
        CHECK(tok.next() == "");
    }

    SECTION("String without delim") {
        tok.setString("bla");
        CHECK(tok.next() == "bla");
    }

    SECTION("String with only delim") {
        tok.setString("{");
        CHECK(tok.next() == "");
    }

    SECTION("String with only same delims") {
        tok.setString("{{");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "{");
        CHECK(tok.next() == "");
    }

    SECTION("String with only delim pair") {
        tok.setString("{}");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "");
    }

    SECTION("One part delim") {
        tok.setString("{bla1}");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "");
    }

    SECTION("Two parts alternating delims") {
        tok.setString("{bla1}bla2");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla1");
        CHECK(tok.next() == "bla2");
        CHECK(tok.next() == "");
    }

    SECTION("Two parts non alternating delims") {
        tok.setString("{bla1{bla2");
        CHECK(tok.next() == "");
        CHECK(tok.next() == "bla1{bla2");
        CHECK(tok.next() == "");
    }
}

TEST_CASE("Tokenizer - Escaping quotes", "[tokenizer]") {
    auto tok = glib::IO::Tokenizer(",");
    tok.setQuote("'");

    SECTION("String without quotes") {
        tok.setString("bla");
        CHECK(tok.next() == "bla");
    }

    SECTION("String with quotes") {
        tok.setString("bla 'quote, quote' bla");
        CHECK(tok.next() == "bla 'quote, quote' bla");
    }

    SECTION("String with quotes, multipart") {
        tok.setString("bla 'quote, quote' bla, and bla");
        CHECK(tok.next() == "bla 'quote, quote' bla");
        CHECK(tok.next() == " and bla");
    }

    SECTION("String with quotes, multipart and multiquote") {
        tok.setString("bla 'quote, quote' bla, and bla 'and, quote', final bla");
        CHECK(tok.next() == "bla 'quote, quote' bla");
        CHECK(tok.next() == " and bla 'and, quote'");
        CHECK(tok.next() == " final bla");
    }
}

TEST_CASE("Tokenizer - by explicit sentinel value", "[tokenizer]") {
    auto tok = glib::IO::Tokenizer(std::vector{":"s, ","s});
    tok.setQuote("'");

    SECTION("one sentinel") {
        tok.setString("{key: value, list: [1, 2]}");
        CHECK(tok.next()    == "{key");
        CHECK(tok.next()    == " value");
        CHECK(tok.next()    == " list");
        CHECK(tok.next("]") == " [1, 2");
    }

    SECTION("two sentinels") {
        tok.setString("{key: value, list: [1, 2]},{key2: value2, list2: [3, 4]}");
        CHECK(tok.next()    == "{key");
        CHECK(tok.next()    == " value");
        CHECK(tok.next()    == " list");
        CHECK(tok.next("]") == " [1, 2");

        CHECK(tok.next()    == "}");

        CHECK(tok.next()    == "{key2");
        CHECK(tok.next()    == " value2");
        CHECK(tok.next()    == " list2");
        CHECK(tok.next("]") == " [3, 4");
    }
}

TEST_CASE("Tokenizer - escapers", "[tokenizer]") {
    auto tok = glib::IO::Tokenizer(",");
    tok.setEscapers({{{"["},{"]"}}});
    tok.setQuote("\"");

    SECTION("single delim, one pair of escaper") {
        tok.setString("a,b,[1,2,3,4],c,d");
        CHECK(tok.next() == "a");
        CHECK(tok.next() == "b");
        CHECK(tok.next() == "1,2,3,4");
        CHECK(tok.next() == "c");
        CHECK(tok.next() == "d");
    }

    SECTION("single delim, one pair of escaper, quoted") {
        tok.setString("a,b,[1,2,\"3,4\"],c,d");
        CHECK(tok.next() == "a");
        CHECK(tok.next() == "b");

        auto lst = tok.next();
        CHECK(lst == "1,2,\"3,4\"");
        CHECK(tok.next() == "c");
        CHECK(tok.next() == "d");

        tok.setString(lst);
        CHECK(tok.next() == "1");
        CHECK(tok.next() == "2");
        CHECK(tok.next() == "\"3,4\"");
    }
}

}    // namespace glib::test
