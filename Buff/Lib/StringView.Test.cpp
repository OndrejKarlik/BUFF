#include "Lib/StringView.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("StringView::operator==") {
    CHECK((StringView("") == ""));
    CHECK((StringView("abc") == "abc"));
    CHECK((StringView("abc") != "bc"));
}

TEST_CASE("StringView::find") {
    CHECK(StringView("abcd").find("abcd") == 0);
    CHECK(StringView("abcd").find("abcde") == NULL_OPTIONAL);
    CHECK(StringView("abcd").find("cd") == 2);
    CHECK(StringView("abcdcdcdcdcd").find("cd"));
    CHECK(StringView("abcd").find("cde") == NULL_OPTIONAL);
    CHECK_ASSERT(StringView("abcd").find(""));
    CHECK(StringView("").find("a") == NULL_OPTIONAL);
}

TEST_CASE("StringView::findLast") {
    CHECK(StringView("abcd").findLast("abcd") == 0);
    CHECK(StringView("abcd").findLast("abcde") == NULL_OPTIONAL);
    CHECK(StringView("abcd").findLast("cd") == 2);
    CHECK(StringView("abcdcdcdcdcd").findLast("cd") == 10);
    CHECK(StringView("abcd").findLast("cde") == NULL_OPTIONAL);
    CHECK_ASSERT(StringView("abcd").findLast(""));
    CHECK(StringView("").findLast("a") == NULL_OPTIONAL);
}

TEST_CASE("StringView::getSubstring") {
    CHECK(StringView("abcd").getSubstring(0, 0) == "");
    CHECK(StringView("abcd").getSubstring(0, 1) == "a");
    CHECK(StringView("abcd").getSubstring(0, 2) == "ab");
    CHECK(StringView("abcd").getSubstring(0, 3) == "abc");
    CHECK(StringView("abcd").getSubstring(0, 4) == "abcd");
    CHECK(StringView("abcd").getSubstring(0) == "abcd");
    CHECK_ASSERT(StringView("abcd").getSubstring(0, 5));
    CHECK(StringView("abcd").getSubstring(1, 0) == "");
    CHECK(StringView("abcd").getSubstring(1, 1) == "b");
    CHECK(StringView("abcd").getSubstring(1, 2) == "bc");
    CHECK(StringView("abcd").getSubstring(1, 3) == "bcd");
    CHECK(StringView("abcd").getSubstring(1) == "bcd");
    CHECK_ASSERT(StringView("abcd").getSubstring(1, 4));
    CHECK(StringView("abcd").getSubstring(2, 0) == "");
    CHECK(StringView("abcd").getSubstring(2, 1) == "c");
    CHECK(StringView("abcd").getSubstring(2, 2) == "cd");
    CHECK(StringView("abcd").getSubstring(2) == "cd");
    CHECK_ASSERT(StringView("abcd").getSubstring(2, 3));
    CHECK(StringView("abcd").getSubstring(3, 0) == "");
    CHECK(StringView("abcd").getSubstring(3, 1) == "d");
    CHECK(StringView("abcd").getSubstring(3) == "d");
    CHECK_ASSERT(StringView("abcd").getSubstring(3, 2) == "d");
    CHECK(StringView("abcd").getSubstring(4, 0) == "");
    CHECK(StringView("abcd").getSubstring(4) == "");
    CHECK_ASSERT(StringView("abcd").getSubstring(4, 1) == "");
    CHECK_ASSERT(StringView("abcd").getSubstring(5));
    CHECK_ASSERT(StringView("abcd").getSubstring(5, 0));
    CHECK_ASSERT(StringView("abcd").getSubstring(5, 1));
    CHECK_ASSERT(StringView("abcd").getSubstring(-1));
    CHECK_ASSERT(StringView("abcd").getSubstring(1, -1));
}

static_assert(IsEqualsComparable<StringView>);
// static_assert(IsThreeWayComparable<StringView>); Emscripten does not support this

static_assert(IsEqualsComparable<StringView, const char*>);
static_assert(IsEqualsComparable<StringView, String>);

BUFF_NAMESPACE_END
