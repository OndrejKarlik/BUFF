#include "Lib/containers/StableArray.h"
#include "Lib/Bootstrap.Test.h"
#include <doctest/doctest.h>

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class StableArray<int>;
template class StableArray<float>;
template class StableArray<String>;
template class StableArray<String*>;
template class StableArray<NoncopyableMovable>;
// TODO: Make this work template class StableArray<Noncopyable>;

TEST_CASE("StableArray::pushBack") {
    StableArray<int> a({.granularity = 3});
    for (const int i : range(20)) {
        a.pushBack(i);
    }
    CHECK(a.size() == 20);
    int index = 0;
    for (auto& element : a) {
        CHECK(element == index++);
    }
}

TEST_CASE("StableArray with String") {
    StableArray<String> a({.granularity = 3});
    for (const int i : range(20)) {
        a.pushBack(toStr(i));
    }
    CHECK(a.size() == 20);
    int index = 0;
    for (auto& element : a) {
        CHECK(element == toStr(index++));
    }
    for (const int i : range(20)) {
        CHECK(a.back() == toStr(19 - i));
        a.popBack();
    }
    CHECK(a.size() == 0);
}

TEST_CASE("StableArray with NoncopyableMovable") {
    StableArray<NoncopyableMovable> a({.granularity = 3});
    for ([[maybe_unused]] const int i : range(20)) {
        a.pushBack();
    }
    CHECK(a.size() == 20);
    for ([[maybe_unused]] const int i : range(20)) {
        a.popBack();
    }
    CHECK(a.size() == 0);

    const StableArray<NoncopyableMovable> b = std::move(a);
}

TEST_CASE("StableArray::popBack") {
    StableArray<int> a({.granularity = 3});
    for (const int i : range(20)) {
        a.pushBack(i);
    }
    CHECK(a.size() == 20);
    for (const int i : range(20)) {
        CHECK(a.back() == 19 - i);
        a.popBack();
    }
    CHECK(a.size() == 0);
}

TEST_CASE("StableArray stability") {
    StableArray<String> array({.granularity = 2});

    array.pushBack("1");
    const String* a = &array.back();
    array.pushBack("2");
    const String* b = &array.back();
    array.pushBack("3");
    const String* c = &array.back();
    for (int i = 0; i < 10; ++i) {
        array.pushBack("more");
    }
    CHECK(&array[0] == a);
    CHECK(&array[1] == b);
    CHECK(&array[2] == c);
}

TEST_CASE("StableArray reverse iteration") {
    StableArray<String> array({.granularity = 2});
    for (const int i : range(20)) {
        array.pushBack(toStr(i));
    }
    int index = 20;
    for (auto it : iterateReverse(array)) {
        CHECK(it == toStr(--index));
    }
    CHECK(index == 0);
}

BUFF_NAMESPACE_END
