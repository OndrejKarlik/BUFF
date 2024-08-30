#include "Lib/Json.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("escapeJson") {
    CHECK(escapeJson("abc") == "abc");
    CHECK(escapeJson("a\"bc") == "a\\\"bc");
    CHECK(escapeJson("a\\bc") == "a\\\\bc");
    CHECK(escapeJson("a\\\"bc") == "a\\\\\\\"bc");
    CHECK(escapeJson("a\tbc") == "a\\tbc");
    CHECK(escapeJson("a@{}[]()'-.,bc") == "a@{}[]()'-.,bc");
    CHECK(escapeJson("\x01\x12\x0F\x1F") == "\\u0001\\u0012\\u000f\\u001f");
    CHECK(escapeJson(u8"abcřžýxyz") == u8"abcřžýxyz");
}

TEST_CASE("unEscapeJson") {
    const Array DATA = {
        "abcd",
        "a b + 9K",
        "a\"b\\c\\\"d",
        "a \b b \t c \r d \n e \f f \v g",
        "\x01 a \x12 b \x0F c \x1F",
    };
    DOCTEST_VALUE_PARAMETERIZED_DATA(value, DATA);
    CHECK(unEscapeJson(escapeJson(value)) == value);
}

TEST_CASE("jsonRoundTrip") {
    CHECK_STREQ(parseJson("{}")->toJson(), "{}");
    CHECK_STREQ(parseJson(R"({ "A" : "" })")->toJson(), R"({"A": ""})");
    CHECK_STREQ(parseJson(R"({ "A" : "\"" })")->toJson(), R"({"A": "\""})");
    CHECK_STREQ(parseJson(R"({ "A" : "\\\"" })")->toJson(), R"({"A": "\\\""})");
    CHECK_STREQ(parseJson(" { \"a\" : 1 }  ")->toJson(), "{\"a\": 1}");
    CHECK_STREQ(
        parseJson(R"({
    "boolean_False" : false ,
    "boolean_true" : true   ,
    "null" : null,
  "number"   : 123456 ,
  "string" : "st r \\\" \\"
})")
            ->toJson(),
        R"({"boolean_False": false, "boolean_true": true, "null": null, "number": 123456, "string": "st r \\\" \\"})");

    CHECK_STREQ(parseJson(R"({"obj" : { "A" : 123 } })")->toJson(), R"({"obj": {"A": 123}})");

    CHECK_STREQ(parseJson(R"({"arr" : [ 1 , 3 , true , "abc" ] })")->toJson(),
                R"({"arr": [1, 3, true, "abc"]})");
}

BUFF_NAMESPACE_END
