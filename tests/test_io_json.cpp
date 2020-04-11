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

    for (const auto& [key, value] : rec) {
        std::cout << "Key   : " << key << '\n';
    }

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

}    // namespace glib::test
