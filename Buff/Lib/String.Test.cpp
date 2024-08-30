#include "Lib/String.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/containers/Array.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("String constructors") {
    // Empty
    CHECK(String().isEmpty());

    // Char*
    CHECK(String("").isEmpty());
    CHECK(String("abc", 0).isEmpty());
    CHECK(StringView(String("abc")).size() == 3);
    CHECK(StringView(String("abc")) == "abc");
    CHECK(StringView(String("abcdef", 3)) == "abc");

    // std::string
    CHECK(StringView(String(std::string("qwe"))) == "qwe");

    // std::wstring
    CHECK(String(std::wstring(L"\u010D\u0065\u0072\u0148")) == "\xC4\x8D\x65\x72\xC5\x88");

    // char8_t*
    CHECK(String(u8"").isEmpty());
    CHECK(String(u8"abc", 0).isEmpty());
    CHECK(StringView(String(u8"yxc")).size() == 3);
    CHECK(StringView(String(u8"yxc")) == "yxc");
    CHECK(StringView(String(u8"abcdef", 3)) == "abc");
}

TEST_CASE("String operator=") {
    String str;

    // String
    str = String {};
    CHECK(str.isEmpty());
    str = String("abc");
    CHECK(str == "abc");

    // Char*
    str = "";
    CHECK(str.isEmpty());
    str = "abc";
    CHECK(str == "abc");

    // std::string
    str = std::string("xyz");
    CHECK(str == "xyz");

    // std::wstring
    str = std::wstring(L"\u010D\u0065\u0072\u0148");
    CHECK(str == "\xC4\x8D\x65\x72\xC5\x88");

    // char8_t*
    str = u8"";
    CHECK(str.isEmpty());
    str = u8"qwe";
    CHECK(str == "qwe");
}

TEST_CASE("String bug init") {
    String x = "abc";
    x        = String {};
    CHECK(x.isEmpty());
}

TEST_CASE("String assign StringView") {
    String x = "abc";
    x        = StringView("xyz");
    CHECK_STREQ(x, "xyz");
    CHECK_STREQ(x.asCString(), "xyz");

    x = "012";
    CHECK_STREQ(x, "012");
    CHECK_STREQ(x.asCString(), "012");

    // There was a bug:
    x = x.getSubstring(1, 1);
    CHECK_STREQ(x, "1");
}

TEST_CASE("String::asCString") {
    CHECK(std::strcmp(String("").asCString(), "") == 0);
    CHECK(std::strcmp(String("d").asCString(), "d") == 0);
    CHECK(std::strcmp(String("abc").asCString(), "abc") == 0);
}

TEST_CASE("String utf8->utf16") {
    CHECK(String("ěščřžýáíé").asWString() == std::wstring(L"ěščřžýáíé"));
    CHECK(String("🤣💕👌").asWString() == std::wstring(L"🤣💕👌"));
}

TEST_CASE("String cp1252->utf8") {
    CHECK(String::fromCp1252("abcdyz091[]*)=") == "abcdyz091[]*)=");
    CHECK(String::fromCp1252("\xA1\xFF\xD4") == u8"¡ÿÔ");
}

TEST_CASE("String::operator<=>") {
    CHECK(((String("") <=> String("")) == 0));
    CHECK(((String("a") <=> String("a")) == 0));
    CHECK(((String("a") <=> String("b")) < 0));
    CHECK(((String("b") <=> String("a")) > 0));
    CHECK(((String("a") <=> String("aa")) < 0));
    CHECK(((String("aa") <=> String("a")) > 0));
    CHECK(((String("a") <=> String("ab")) < 0));
    CHECK(((String("ab") <=> String("a")) > 0));
    CHECK(((String("ab") <=> String("ac")) < 0));
    CHECK(((String("ac") <=> String("ab")) > 0));
    CHECK(((String("ab") <=> String("abc")) < 0));
}
TEST_CASE("String::operator<=> against char*") {
    CHECK(((String("") <=> "") == 0));
    CHECK(((String("a") <=> "a") == 0));
    CHECK(((String("a") <=> "b") < 0));
    CHECK(((String("b") <=> "a") > 0));
    CHECK(((String("a") <=> "aa") < 0));
    CHECK(((String("aa") <=> "a") > 0));
    CHECK(((String("a") <=> "ab") < 0));
    CHECK(((String("ab") <=> "a") > 0));
    CHECK(((String("ab") <=> "ac") < 0));
    CHECK(((String("ac") <=> "ab") > 0));
    CHECK(((String("ab") <=> "abc") < 0));
}

TEST_CASE("String::operator==") {
    CHECK(String("") == String(""));
    CHECK(String("a") == String("a"));
    CHECK(String("a") != String("b"));
    CHECK(String("a") != String("aa"));
    CHECK(String("aa") != String("a"));
    CHECK(String("a") != String("ab"));
    CHECK(String("ab") != String("a"));
    CHECK(String("ab") != String("ac"));
    CHECK(String("ac") != String("ab"));
    CHECK(String("ab") != String("abc"));
    CHECK(String("ab") != "abc");
    CHECK(String("ab") == "ab");
    CHECK(String("") == "");
}

TEST_CASE("String::asWString") {
    CHECK(String("").asWString() == L"");
    CHECK(String("d").asWString() == L"d");
    CHECK(String("abc").asWString() == L"abc");
    CHECK(String("\xC4\x8D\x65\x72\xC5\x88").asWString() == std::wstring(L"\u010D\u0065\u0072\u0148"));
}

TEST_CASE("String::operator[]") {
    CHECK(String("abc")[0] == 'a');
    CHECK(String("abc")[1] == 'b');
    CHECK(String("abc")[2] == 'c');
    CHECK_ASSERT(String("abc")[3]);
    CHECK_ASSERT(String("abc")[4]);
    CHECK_ASSERT(String("abc")[-1]);
    CHECK_ASSERT(String("abc")[-2]);
}

TEST_CASE("String::operator+=") {
    String x;
    x += "a";
    CHECK(x == "a");
    x += "b";
    CHECK(x == "ab");
    x += "c";
    CHECK(x == "abc");
    x += "rew";
    CHECK(x == "abcrew");
}

TEST_CASE("String::operator<<") {
    String x;
    x << "a";
    CHECK(x == "a");
    x << "b";
    CHECK(x == "ab");
    x << 'c';
    CHECK(x == "abc");
}

TEST_CASE("String::getWithReplaceAll") {
    CHECK(String("abcdeafa").getWithReplaceAll("a", "X") == "XbcdeXfX");
    CHECK(String("abcdeaf").getWithReplaceAll("aa", "X") == "abcdeaf");
    CHECK(String("abcdeafa").getWithReplaceAll("a", "") == "bcdef");
    CHECK(String("abcdeafa").getWithReplaceAll("abcdeafa", "") == "");
    CHECK(String("abcdeafa").getWithReplaceAll("abcdeafaw", "ss") == "abcdeafa");
}

TEST_CASE("String::getTrimmed") {
    CHECK(String("").getTrimmed() == "");
    CHECK(String(" ").getTrimmed() == "");
    CHECK(String(" \n\t    \n ").getTrimmed() == "");
    CHECK(String(" \n\t  a b \n ").getTrimmed() == "a b");
    CHECK(String("a b \n ").getTrimmed() == "a b");
    CHECK(String(" \n\t  a b").getTrimmed() == "a b");
    CHECK(String("a b \nc").getTrimmed() == "a b \nc");
}

TEST_CASE("String::findFirstOf") {
    CHECK(!String("").findFirstOf(""));
    CHECK(!String("").findFirstOf("abc"));
    CHECK(!String("def").findFirstOf("abc"));
    CHECK(String("def").findFirstOf("efg") == 1);
    CHECK(String("0123456789").findFirstOf("7") == 7);
    CHECK(String("0123456789").findFirstOf("abcdfeg8") == 8);
}

TEST_CASE("String::erase") {
    CHECK_ASSERT(String("").getWithErase(0));
    CHECK_ASSERT(String("").getWithErase(0, 0));
    CHECK_ASSERT(String("").getWithErase(0, 1));
    CHECK_ASSERT(String("").getWithErase(1, 0));
    CHECK_ASSERT(String("").getWithErase(1, 1));
    CHECK(String("a").getWithErase(0, 0) == "a");
    CHECK(String("a").getWithErase(0, 1) == "");
    CHECK(String("a").getWithErase(0) == "");
    CHECK_ASSERT(String("a").getWithErase(1, 0));
    CHECK_ASSERT(String("a").getWithErase(1, 1));
    CHECK(String("abcd").getWithErase(1, 2) == "ad");
    CHECK(String("abcd").getWithErase(1, 3) == "a");
    CHECK(String("abcd").getWithErase(0) == "");
    CHECK(String("abcd").getWithErase(2) == "ab");
    CHECK(String("abcd").getWithErase(0, 1) == "bcd");
    CHECK(String("abcd").getWithErase(0, 2) == "cd");
    CHECK(String("abcd").getWithErase(0, 3) == "d");
    CHECK(String("abcd").getWithErase(0, 4) == "");
    CHECK(String("abcd").getWithErase(1, 1) == "acd");
    CHECK(String("abcd").getWithErase(1, 2) == "ad");
    CHECK(String("abcd").getWithErase(1, 3) == "a");
    CHECK(String("abcd").getWithErase(2, 1) == "abd");
    CHECK(String("abcd").getWithErase(2, 2) == "ab");
    CHECK(String("abcd").getWithErase(2, 3) == "ab");
    CHECK(String("abcd").getWithErase(3, 1) == "abc");
    CHECK_ASSERT(String("abcd").getWithErase(4));
    CHECK_ASSERT(String("abcd").getWithErase(4, 0));
    CHECK_ASSERT(String("abcd").getWithErase(4, 1));
    CHECK_ASSERT(String("abcd").getWithErase(-1));
    CHECK_ASSERT(String("abcd").getWithErase(-1, 0));
    CHECK_ASSERT(String("abcd").getWithErase(-1, 1));
}

TEST_CASE("String::insert") {
    CHECK_ASSERT("abcd"_S.getWithInsert(-1, "a"));
    CHECK_ASSERT("abcd"_S.getWithInsert(5, "a"));
    CHECK("abcd"_S.getWithInsert(0, "ef") == "efabcd");
    CHECK("abcd"_S.getWithInsert(1, "ef") == "aefbcd");
    CHECK("abcd"_S.getWithInsert(4, "ef") == "abcdef");
    CHECK("abcd"_S.getWithInsert(4, "") == "abcd");
    CHECK("abcd"_S.getWithInsert(0, "") == "abcd");
    CHECK("abcd"_S.getWithInsert(0, "e") == "eabcd");
}

TEST_CASE("String::replace") {
    CHECK_ASSERT("abcd"_S.getWithReplace(-1, 1, "a"));
    CHECK_ASSERT("abcd"_S.getWithReplace(5, 1, "a"));
    CHECK_ASSERT("abcd"_S.getWithReplace(1, -1, "a"));
    CHECK("abcd"_S.getWithReplace(0, 0, "") == "abcd");
    CHECK("abcd"_S.getWithReplace(0, 1, "") == "bcd");
    CHECK("abcd"_S.getWithReplace(0, 2, "") == "cd");
    CHECK("abcd"_S.getWithReplace(1, 2, "") == "ad");
    CHECK("abcd"_S.getWithReplace(1, 3, "BCD") == "aBCD");
    CHECK("abcd"_S.getWithReplace(1, 0, "BCD") == "aBCDbcd");
    CHECK("abcd"_S.getWithReplace(0, 0, "BCD") == "BCDabcd");
    CHECK("abcd"_S.getWithReplace(0, 4, "BCD") == "BCD");
    CHECK("abcd"_S.getWithReplace(0, 40, "BCD") == "BCD");
    CHECK("abcd"_S.getWithReplace(4, 40, "BCD") == "abcdBCD");
    CHECK("abcd"_S.getWithReplace(4, 0, "BCD") == "abcdBCD");
}

TEST_CASE("String::getToLower") {
    CHECK(String("").getToLower() == "");
    CHECK(String("abcd").getToLower() == "abcd");
    CHECK(String(" 1093 . -)@&|~\n").getToLower() == " 1093 . -)@&|~\n");
    CHECK(String("abcdEFGH").getToLower() == "abcdefgh");
}

TEST_CASE("fromStr") {
    CHECK(fromStr<int>("0") == 0);
    CHECK(fromStr<int>("10") == 10);
    CHECK(fromStr<int>("-10") == -10);
}

TEST_CASE("toStr doubles") {
    // Testing exact print of decimal places beyond what streams print by default. The constant tested is
    // 1/2^20 - a number that is exactly representable as double
    CHECK_STREQ(toStr(1.00000095367431640625), "1.00000095367431640625");
    CHECK_STREQ(toStr(14695031469503.0), "14695031469503");
}

TEST_CASE("parseNumber") {
    CHECK(parseNumber<int>("0", 8) == 0);
    CHECK(parseNumber<int>("0", 10) == 0);
    CHECK(parseNumber<int>("0", 16) == 0);

    CHECK(parseNumber<int>("10", 16) == 0x10);
    CHECK(parseNumber<int>("-10", 16) == -0x10);
    CHECK(parseNumber<int>("f", 16) == 0xf);
    CHECK(parseNumber<int>("fF", 16) == 0xff);

    CHECK(parseNumber<int>("10", 8) == 8);
    CHECK(parseNumber<int>("-10", 8) == -8);
    CHECK(parseNumber<int>("24", 8) == 2 * 8 + 4);

    CHECK(parseNumber<int>("123", 10) == 123);
    CHECK(parseNumber<int>("-123", 10) == -123);

    CHECK(!parseNumber<int>("", 10));
    CHECK(!parseNumber<int>("F", 10));
    CHECK(!parseNumber<int>("a", 10));
    CHECK(!parseNumber<int>("8", 8));
    CHECK(!parseNumber<int>("Z", 16));
    CHECK(!parseNumber<int>(" ", 10));
}

TEST_CASE("formatFloat") {
    CHECK(formatFloat(42, {.maxDecimals = 2}) == "42");
    CHECK(formatFloat(42, {.maxDecimals = 2, .forceDecimals = true}) == "42.00");
    CHECK(formatFloat(1.1234, {.maxDecimals = 2}) == "1.12");
    CHECK(formatFloat(1.1234, {.maxDecimals = 10}) == "1.1234");
    CHECK(formatFloat(1.1234, {.maxDecimals = 6, .forceDecimals = true}) == "1.123400");
    CHECK(formatFloat(12345, {.maxDecimals = 6, .forceDecimals = true}) == "12345.000000");

    CHECK(formatFloat(-42, {.maxDecimals = 2}) == "-42");
    CHECK(formatFloat(-42, {.maxDecimals = 2, .forceDecimals = true}) == "-42.00");
    CHECK(formatFloat(-1.1234, {.maxDecimals = 2}) == "-1.12");
    CHECK(formatFloat(-1.1234, {.maxDecimals = 10}) == "-1.1234");
    CHECK(formatFloat(-1.1234, {.maxDecimals = 6, .forceDecimals = true}) == "-1.123400");
    CHECK(formatFloat(-12345, {.maxDecimals = 6, .forceDecimals = true}) == "-12345.000000");
}

TEST_CASE("String::explode") {
    CHECK_ASSERT(String("").explode(""));
    CHECK(String("").explode("a") == Array<StringView>(1));
    CHECK(String("aA").explode("aA") == Array {StringView {}, StringView {}});
    CHECK(String("aa").explode("a") == Array {StringView {}, StringView {}, StringView {}});
    CHECK(String("a a").explode(" ") == Array {StringView("a"), StringView("a")});
    CHECK(String("ein zwei drei ").explode(" ") ==
          Array {StringView("ein"), StringView("zwei"), StringView("drei"), StringView {}});
    CHECK(String("aDELIMITa").explode("DELIMIT") == Array {StringView("a"), StringView("a")});
}

TEST_CASE("String isLetter") {
    for (const int i : range(128)) {
        CHECK(isLetter(char(i)) == (std::isalpha(i) != 0));
    }
}

TEST_CASE("String listToStr") {
    CHECK(listToStr(Array<String> {}) == "");
    CHECK(listToStr(Array<String> {"a"}) == "a");
    CHECK(listToStr(Array<String> {"a", "b"}) == "a b");
    CHECK(listToStr(Array<String> {"a", "b", "c"}) == "a b c");
    CHECK(listToStr(Array<String> {"a", "b", "c"}, "|") == "a|b|c");
}

TEST_CASE("String getLevenshteinDistance") {
    CHECK(getLevenshteinDistance("", "").distance == 0);
    CHECK(getLevenshteinDistance("a", "").distance == 1);
    CHECK(getLevenshteinDistance("", "a").distance == 1);
    CHECK(getLevenshteinDistance("a", "a").distance == 0);
    CHECK(getLevenshteinDistance("a", "b").distance == 1);
    CHECK(getLevenshteinDistance("a", "ab").distance == 1);
    CHECK(getLevenshteinDistance("ab", "a").distance == 1);
    CHECK(getLevenshteinDistance("ab", "b").distance == 1);
    CHECK(getLevenshteinDistance("ab", "ba").distance == 2);
    CHECK(getLevenshteinDistance("abc", "abc").distance == 0);
    CHECK(getLevenshteinDistance("abc", "abcd").distance == 1);
    CHECK(getLevenshteinDistance("abc", "abdc").distance == 1);
    CHECK(getLevenshteinDistance("abc", "ab").distance == 1);
    CHECK(getLevenshteinDistance("abc", "ac").distance == 1);
    CHECK(getLevenshteinDistance("abc", "bc").distance == 1);
    CHECK(getLevenshteinDistance("abc", "ac").distance == 1);
    CHECK(getLevenshteinDistance("abc", "ac").distance == 1);
    CHECK(getLevenshteinDistance("abc", "abctghzu").distance == 5);

    // Longer examples:
    CHECK(getLevenshteinDistance("laska", "pivo").distance == 5);
    CHECK(getLevenshteinDistance("kitten", "sitting").distance == 3);
    CHECK(getLevenshteinDistance("afoo", "foof").distance == 2);
    CHECK(getLevenshteinDistance("afoo", "").distance == 4);
    CHECK(getLevenshteinDistance("", "12345").distance == 5);

    CHECK(getLevenshteinDistance("The quick brown fox jumps over the lazy dog.",
                                 "A quick brown fox jumps over a lazy dog.")
              .distance == 6);
    CHECK(getLevenshteinDistance("The quick brown dog jumps over the lazy fox.",
                                 "The fast brown fox jumps over the lazy dog.")
              .distance == 9);
    CHECK(getLevenshteinDistance("aDaD", "a").distance == 3);
}

TEST_CASE("String getLevenshteinDistance reconstruction") {
    struct Data {
        String        from;
        String        to;
        Optional<int> numChanges; // Includes effects of batching. NOT levenshtein distance!
    };
    const std::initializer_list<Data> PAIRS = {
        // Empty/Trivial
        {"",                                                                 "",                                                                 0            },
        {"abc",                                                              "abc",                                                              0            },

        // Substitution only
        {"a",                                                                "b",                                                                1            },
        {"foo",                                                              "bar",                                                              1            },
        {"1234",                                                             "4231",                                                             2            },
        {"qwertz",                                                           "asdfgh",                                                           1            },
        {"qweYXCVrtz",                                                       "asdYXCVfgh",                                                       2            },

        // Delete from beginning
        {"abcdef",                                                           "bcdef",                                                            1            },
        {"abcdef",                                                           "cdef",                                                             1            },
        {"abcdef",                                                           "ef",                                                               1            },
        {"abcdef",                                                           "",                                                                 1            },

        // Insert at the beginning
        {"bcdef",                                                            "abcdef",                                                           1            },
        {"cdef",                                                             "abcdef",                                                           1            },
        {"ef",                                                               "abcdef",                                                           1            },
        {"",                                                                 "abcdef",                                                           1            },

        // Insert at the end
        {"a",                                                                "ab",                                                               1            },
        {"a",                                                                "abc",                                                              1            },
        {"abc",                                                              "abcd",                                                             1            },
        {"012",                                                              "0123",                                                             1            },
        {"012",                                                              "01234",                                                            1            },
        {"012",                                                              "01234567890",                                                      1            },

        {"a",                                                                "",                                                                 1            },
        {"a",                                                                "ab",                                                               1            },
        {"ab",                                                               "a",                                                                1            },
        {"ab",                                                               "b",                                                                1            },
        {"ab",                                                               "ba",                                                               1            },
        {"ab",                                                               "cd",                                                               1            },
        {"abc",                                                              "abc",                                                              0            },
        {"abc",                                                              "abcd",                                                             1            },
        {"abc",                                                              "abdc",                                                             1            },
        {"abc",                                                              "ab",                                                               1            },
        {"abc",                                                              "ac",                                                               1            },
        {"abc",                                                              "bc",                                                               1            },
        {"abc",                                                              "ac",                                                               1            },
        {"abc",                                                              "ac",                                                               1            },
        {"laska",                                                            "pivo",                                                             2            },
        {"kitten",                                                           "sitting",                                                          3            },
        {"afoo",                                                             "foof",                                                             2            },
        {"FooBarFoo",                                                        "BarFooBarBarFooFoo",                                               3            },
        {"afoo",                                                             "",                                                                 1            },
        {"",                                                                 "12345",                                                            1            },
        {"",                                                                 "12345",                                                            1            },
        {"ea58753b12aee18b04cb81cdbaf98c57cd26e574a568096168df9dd374840ea6",
         "fd19bece3128cad2593ee069747c8aa136b71efa58a31a2a34e0b70a097fc911",                                                                     NULL_OPTIONAL},
        {"",                                                                 "fd19bece3128cad2593ee069747c8aa136b71efa58a31a2a34e0b70a097fc911", NULL_OPTIONAL},
        {"fd19bece3128cad2593ee069747c8aa136b71efa58a31a2a34e0b70a097fc911", "",                                                                 NULL_OPTIONAL},

        {"aDaD",                                                             "a",                                                                1            },
        {"a",                                                                "aDaD",                                                             1            },
        {"The foobar",                                                       "A foobaz",                                                         3            },
        {"The quick brown fox jumps over the lazy dog.",                     "A quick brown fox jumps over a lazy dog.",                         4            },
        {"The quick brown fox jumps over the lazy dog.",                     "The fast brown fox jumps over the lazy dog.",                      2            },
        // {"Normal deleted more identical Normal substituted", "Normal more identical Inserted Normal
        // SUBSTITUTED", 3}
    };
    DOCTEST_VALUE_PARAMETERIZED_DATA(data, PAIRS);

    String from = data.from;
    CAPTURE(from);
    String to = data.to;
    CAPTURE(to);
    const LevenshteinDistance distance = getLevenshteinDistance(from, to);
    if (data.numChanges) {
        CHECK(*data.numChanges == distance.changes.size());
    }
    String toFormed = from;
    for (const auto& change : distance.changes) {
        change.applyToFrom(toFormed, to);
    }
    CAPTURE(toFormed);
    CHECK(toFormed == to);
}

/// Tests printouts of CHECK_STREQ test. Every test fails on purpose here
TEST_CASE("CHECK_STREQ test printing of strings" * doctest::skip(1)) {
    CHECK_STREQ("aX\nXD", "aD");
    CHECK_STREQ("aDaD", "a");

    CHECK_STREQ("Normal deleted more identical Normal substituted",
                "Normal more identical Inserted Normal SUBSTITUTED");

    CHECK_STREQ("01234567 deleted more identical Normal substituted",
                "01234567 more identical Inserted Normal SUBSTITUTED");

    CHECK_STREQ("The foobar", "A foobaz");

    CHECK_STREQ("abc", "c");
    CHECK_STREQ("abc", "a");
    CHECK_STREQ("a", "abc");
    CHECK_STREQ("c", "abc");
    CHECK_STREQ("ABCD", "0123");
    CHECK_STREQ("ABCD", "ABxxCD");
    CHECK_STREQ("ABxxCD", "ABCD");
    CHECK_STREQ("The quick brown fox jumps over the lazy dog.", "A quick brown fox jumps over a lazy dog.");
    CHECK_STREQ("The quick brown dog jumps over the lazy fox.",
                "The fast brown fox jumps over the lazy dog.");
    CHECK_STREQ("kitten", "sitting");
    CHECK_STREQ("google", "facebook");
    CHECK_STREQ("pivo", "laska");
    CHECK_STREQ("FooBarFoo", "BarFooBarBarFooFoo");
    CHECK_STREQ("ea58753b12aee18b04cb81cdbaf98c57cd26e574a568096168df9dd374840ea6",
                "fd19bece3128cad2593ee069747c8aa136b71efa58a31a2a34e0b70a097fc911");
}

TEST_CASE("leftPad") {
    CHECK_STREQ(leftPad("abc", 4), " abc");
    CHECK_STREQ(leftPad("abc", 5), "  abc");
    CHECK_STREQ(leftPad("abc", 5, '*'), "**abc");
    CHECK_STREQ(leftPad("abc", 3, '*'), "abc");
    CHECK_STREQ(leftPad("abc", 1, '*'), "abc");
    CHECK_ASSERT(leftPad("abc", -1, '*'));
}

static_assert(IsEqualsComparable<String>);
// static_assert(IsThreeWayComparable<String>); Emscripten does not support this

static_assert(IsEqualsComparable<String, const char*>);

BUFF_NAMESPACE_END
