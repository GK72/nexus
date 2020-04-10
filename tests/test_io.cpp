#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "io.h"

using namespace std::literals::string_literals;

namespace glib::test {

TEST_CASE("One delim", "[tokenizer]") {
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

TEST_CASE("One delim, multiple chars", "[tokenizer][!mayfail]") {
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

TEST_CASE("Two delims", "[tokenizer]") {
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

TEST_CASE("Escaping quotes", "[tokenizer]") {
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

}    // namespace glib::test
