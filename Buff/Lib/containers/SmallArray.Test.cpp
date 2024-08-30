#include "Lib/containers/SmallArray.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class SmallArray<int, 3>;
template class SmallArray<float, 3>;
template class SmallArray<String, 3>;
template class SmallArray<String*, 3>;
template class SmallArray<NoncopyableMovable, 1>;
// template class SmallArray<Noncopyable, 1>;

TEST_CASE("SmallArray::pushBack") {
    SmallArray<int, 3> a;
    for (int i : range(20)) {
        a.pushBack(i);
        CHECK(a.back() == i);
    }
    CHECK(a.size() == 20);
    int index = 0;
    for (auto& element : ArrayView<const int>(a)) {
        CHECK(element == index++);
    }
}

TEST_CASE("SmallArray::pushBack string") {
    SmallArray<String, 3> a;
    for (const int i : range(20)) {
        a.pushBack(toStr(i));
        CHECK(a.back() == toStr(i));
    }
    CHECK(a.size() == 20);
    int index = 0;
    for (auto& element : ArrayView<const String>(a)) {
        CHECK(element == toStr(index++));
    }
}

TEST_CASE("SmallArray::popBack") {
    SmallArray<int, 3> a;
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

BUFF_NAMESPACE_END
