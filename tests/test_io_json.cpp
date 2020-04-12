#include "catch.hpp"

#include "io.h"

#ifndef __PATH_ROOT_REPO
#define __PATH_ROOT_REPO ""
#endif

using namespace std::literals::string_literals;
using namespace glib::IO;

const auto pathRootDir    = __PATH_ROOT_REPO;
const auto pathTestInputs = glib::joinStr("/", pathRootDir, "tests/input");
const auto pathTestfile_1 = glib::joinStr("/", pathTestInputs, "test_io_json.json");

namespace glib::test {

std::string_view strip(std::string_view sv)
{
    return trim(sv, std::array<std::string, 3>{ "\n", " ", "\"" });
}

TEST_CASE("Trim", "[tokenizer]") {
    SECTION("Trim whitespace - trailing") {
        CHECK(trim("string ", " ") == "string");
    }

    SECTION("Trim whitespace - leading") {
        CHECK(trim(" string", " ") == "string");
    }

    SECTION("Trim whitespace - both") {
        CHECK(trim(" string ", " ") == "string");
    }

    SECTION("Trim whitespace - both, multiple") {
        CHECK(trim("     string   ", " ") == "string");
    }

    SECTION("Trim multiple = strip") {
        CHECK(strip("     \"string\"   ") == "string");
    }

    SECTION("Trim newline = strip") {
        CHECK(strip("\n     \"string\"   \n") == "string");
    }
}

TEST_CASE("JSON - empty (string)", "[JSON parser - read]") {
    REQUIRE(__PATH_ROOT_REPO != ""s);
    const std::string testInput = "{}";

    ParserJSON<ParseString> json(testInput);

    auto rec = json.readRecord();

    SECTION("Empty") {
        CHECK(rec.size() == 0);
    }
}

TEST_CASE("JSON - one record, one key", "[JSON parser - read]") {
    REQUIRE(__PATH_ROOT_REPO != ""s);
    const std::string testInput = "{\"string\": \"a string\"}";

    ParserJSON<ParseString> json(testInput);

    auto rec = json.readRecord();

    SECTION("Empty") {
        REQUIRE(rec.size() == 1);
        CHECK(std::any_cast<std::string>(rec.at("string")) == "a string");
    }
}

TEST_CASE("JSON - one record, multiple keys", "[JSON parser - read]") {
    REQUIRE(__PATH_ROOT_REPO != ""s);
    const std::string testInput = "\
    {\
        \"string\": \"a string\",\
        \"int\": 1,\
        \"float\": 1.1,\
        \"bool\": true\
    }\
    ";

    ParserJSON<ParseString> json(testInput);

    auto rec = json.readRecord();

    SECTION("Empty") {
        REQUIRE(rec.size() == 4);
        CHECK(std::any_cast<std::string>(rec.at("string")) == "a string");
        CHECK(std::any_cast<int>(rec.at("int")) == 1);
        CHECK(std::any_cast<double>(rec.at("float")) == 1.1);
        CHECK(std::any_cast<bool>(rec.at("bool")) == true);
    }
}

TEST_CASE("JSON - one record with recursive record, multiple keys", "[JSON parser - read]") {
    REQUIRE(__PATH_ROOT_REPO != ""s);
    const std::string testInput = "\
    {\
        \"string\": \"a string\",\
        \"int\": 1,\
        \"float\": 1.1,\
        \"bool\": true,\
        \"record\": {\
            \"inner_1\": \"an inner record\",\
            \"inner_2\": \"another inner record\"\
        }\
    }\
    ";

    ParserJSON<ParseString> json(testInput);

    auto rec = json.readRecord();

    SECTION("Empty") {
        CHECK(rec.size() == 5);
        CHECK(std::any_cast<std::string>(rec.at("string")) == "a string");
        CHECK(std::any_cast<int>(rec.at("int")) == 1);
        CHECK(std::any_cast<double>(rec.at("float")) == 1.1);
        CHECK(std::any_cast<bool>(rec.at("bool")) == true);

        auto inner = std::any_cast<ParserJSON<ParseString>::Record>(rec.at("record"));
        CHECK(std::any_cast<std::string>(inner.at("inner_1")) == "an inner record");
        CHECK(std::any_cast<std::string>(inner.at("inner_2")) == "another inner record");
    }
}

TEST_CASE("JSON - one record, with list", "[JSON parser - read]") {
    REQUIRE(__PATH_ROOT_REPO != ""s);

    SECTION("Only a list") {
        const std::string testInput = "\
        {\
            \"list\": [ 1, 3.3 ]\
        }\
        ";

        ParserJSON<ParseString> json(testInput);

        auto rec = json.readRecord();
        auto list = std::any_cast<std::vector<std::any>>(rec.at("list"));

        REQUIRE(list.size() == 2);
        CHECK(std::any_cast<int>        (list[0]) == 1);
        CHECK(std::any_cast<double>     (list[1]) == 3.3);
    }

    SECTION("Two lists") {
        const std::string testInput = "\
        {\
            \"list\": [ 2, 4.4 ],\
            \"list2\": [ true, false, \"true\" ]\
        }\
        ";

        ParserJSON<ParseString> json(testInput);

        auto rec = json.readRecord();
        auto list = std::any_cast<std::vector<std::any>>(rec.at("list"));
        auto list2 = std::any_cast<std::vector<std::any>>(rec.at("list2"));

        REQUIRE(list.size() == 2);
        REQUIRE(list2.size() == 3);

        CHECK(std::any_cast<int>        (list[0]) == 2);
        CHECK(std::any_cast<double>     (list[1]) == 4.4);
        CHECK(std::any_cast<bool>       (list2[0]) == true);
        CHECK(std::any_cast<bool>       (list2[1]) == false);
        CHECK(std::any_cast<std::string>(list2[2]) == "true");
    }

    SECTION("A list at the end") {
        const std::string testInput = "\
        {\
            \"string\": \"a string\",\
            \"int\": 3,\
            \"float\": 5.5,\
            \"bool\": true,\
            \"list\": [ 3, 0, 5.6, \"a string\", false]\
        }\
        ";

        ParserJSON<ParseString> json(testInput);

        auto rec = json.readRecord();

        REQUIRE(rec.size() == 5);
        CHECK(std::any_cast<std::string>(rec.at("string")) == "a string");
        CHECK(std::any_cast<int>        (rec.at("int"))    == 3);
        CHECK(std::any_cast<double>     (rec.at("float"))  == 5.5);
        CHECK(std::any_cast<bool>       (rec.at("bool"))   == true);

        auto list = std::any_cast<std::vector<std::any>>(rec.at("list"));
        REQUIRE(list.size() == 5);
        CHECK(std::any_cast<int>        (list[0]) == 3);
        CHECK(std::any_cast<int>        (list[1]) == 0);
        CHECK(std::any_cast<double>     (list[2]) == 5.6);
        CHECK(std::any_cast<std::string>(list[3]) == "a string");
        CHECK(std::any_cast<bool>       (list[4]) == false);
    }

    SECTION("A list in the middle") {
        const std::string testInput = "\
        {\
            \"string\": \"a string\",\
            \"int\": 1,\
            \"list\": [ 5, 0, 8.8, \"a string\", false],\
            \"float\": 1.1,\
            \"bool\": true,\
        }\
        ";

        ParserJSON<ParseString> json(testInput);

        auto rec = json.readRecord();

        REQUIRE(rec.size() == 5);
        CHECK(std::any_cast<std::string>(rec.at("string")) == "a string");
        CHECK(std::any_cast<int>        (rec.at("int"))    == 1);
        CHECK(std::any_cast<double>     (rec.at("float"))  == 1.1);
        CHECK(std::any_cast<bool>       (rec.at("bool"))   == true);

        auto list = std::any_cast<std::vector<std::any>>(rec.at("list"));
        REQUIRE(list.size() == 5);
        CHECK(std::any_cast<int>        (list[0]) == 5);
        CHECK(std::any_cast<int>        (list[1]) == 0);
        CHECK(std::any_cast<double>     (list[2]) == 8.8);
        CHECK(std::any_cast<std::string>(list[3]) == "a string");
        CHECK(std::any_cast<bool>       (list[4]) == false);
    }
}



}    // namespace glib::test
