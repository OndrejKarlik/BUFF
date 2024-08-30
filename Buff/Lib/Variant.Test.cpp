#include "Lib/Variant.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class Variant<int, float>;
template class Variant<float, float>;
template class Variant<String, int, float>;
template class Variant<String*, NoncopyableMovable>;

TEST_CASE("Variant construct, holdsType, tryGet") {
    {
        Variant<int, String> v1 = 5;
        CHECK(v1.holdsType<int>());
        CHECK(!v1.holdsType<String>());
        CHECK(*v1.tryGet<int>() == 5);
        CHECK(v1.tryGet<String>() == nullptr);
        CHECK(v1.get<int>() == 5);
        CHECK_ASSERT(v1.get<String>());
    }
    {
        Variant<int, String> v2 = "hello";
        CHECK(!v2.holdsType<int>());
        CHECK(v2.holdsType<String>());
        CHECK(v2.tryGet<int>() == nullptr);
        CHECK(*v2.tryGet<String>() == "hello");
        CHECK_ASSERT(v2.get<int>());
        CHECK(v2.get<String>() == "hello");
    }
}

TEST_CASE("Variant visit") {
    int    sumInt = 0;
    String concatString;
    auto   intFunctor    = [&](const int i) { sumInt += i; };
    auto   stringFunctor = [&](const String& i) { concatString << i; };
    {

        Variant<int, String> v1 = 5;
        v1.visit(intFunctor, stringFunctor);
        CHECK(sumInt == 5);
        v1.visit(stringFunctor, intFunctor);
        CHECK(sumInt == 10);
    }
    {
        Variant<int, String> v2 = "hello";
        v2.visit(intFunctor, stringFunctor);
        CHECK(concatString == "hello");
    }
}

BUFF_NAMESPACE_END
