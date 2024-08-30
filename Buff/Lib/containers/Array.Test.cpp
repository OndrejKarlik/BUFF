#include "Lib/containers/Array.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class Array<int>;
template class Array<float>;
template class Array<String>;
template class Array<String*>;
template class Array<NoncopyableMovable>;

TEST_CASE("Array insert") {
    Array<int> x;
    CHECK_ASSERT(x.insert(-1, 5));
    CHECK_ASSERT(x.insert(1, 5));
    CHECK(x.isEmpty());

    CHECK_NOTHROW(x.insert(0, 5));
    CHECK(x == Array {5});
    CHECK_NOTHROW(x.insert(0, 3));
    CHECK(x == Array {3, 5});
    CHECK_NOTHROW(x.insert(1, 4));
    CHECK(x == Array {3, 4, 5});
    CHECK_NOTHROW(x.insert(3, 6));
    CHECK(x == Array {3, 4, 5, 6});
}

TEST_CASE("Array moves") {
    Array<int> x = {1, 2, 3, 4, 5};
    Array<int> y = std::move(x);
    CHECK(x.isEmpty());
    CHECK(y == Array {1, 2, 3, 4, 5});
    Array<int> z;
    z = std::move(y);
    CHECK(x.isEmpty());
    CHECK(y.isEmpty());
    CHECK(z == Array {1, 2, 3, 4, 5});
}

TEST_CASE("Array ArrayView constructor") {
    Array<int> x = {1, 2, 3, 4, 5};
    CHECK(x.size() == 5);
    for (const int i : range(5)) {
        CHECK(x[i] == i + 1);
    }
    Array<int> z = Array(ArrayView<int>(x));
    CHECK(x == z);
    Array<int> w = Array(ArrayView<const int>(x));
    CHECK(x == w);
}

TEST_CASE("Array compare") {
    Array x = {1, 2, 3, 4, 5};
    Array y = {1, 2, 3, 4, 5};
    Array z = {1, 2, 3, 4, 6};
    CHECK(x == y);
    CHECK(x != z);
    CHECK((Array<int>::threeWayCompare(x, z) < 0));
    CHECK((Array<int>::threeWayCompare(x, x) == 0));
    CHECK((Array<int>::threeWayCompare(x, y) == 0));
    CHECK((Array<int>::threeWayCompare(z, x) > 0));
}

TEST_CASE("Array pushBackRange") {
    Array<int> x;
    x.pushBackRange({1, 2, 3, 4, 5});
    CHECK(x == Array {1, 2, 3, 4, 5});
    const Array y = {5, 6, 7};
    x.pushBackRange(y);
    CHECK(x == Array {1, 2, 3, 4, 5, 5, 6, 7});
    x.pushBackRange({});
    CHECK(x == Array {1, 2, 3, 4, 5, 5, 6, 7});
    Array<int> z;
    z.pushBackRange({});
    CHECK(z.isEmpty());
}

TEST_CASE("Array insertRange") {
    Array<int> x;
    x.insertRange<const int>(0, {1, 2, 3, 4, 5});
    CHECK(x == Array {1, 2, 3, 4, 5});
    x.insertRange<const int>(0, {5, 6, 7});
    CHECK(x == Array {5, 6, 7, 1, 2, 3, 4, 5});
    x.insertRange<const int>(0, {});
    CHECK(x == Array {5, 6, 7, 1, 2, 3, 4, 5});
    x.insertRange<const int>(5, {});
    CHECK(x == Array {5, 6, 7, 1, 2, 3, 4, 5});
    x.insertRange<const int>(x.size(), {});
    CHECK(x == Array {5, 6, 7, 1, 2, 3, 4, 5});
    x.insertRange<const int>(x.size(), {99});
    CHECK(x == Array {5, 6, 7, 1, 2, 3, 4, 5, 99});
    Array<int> z;
    z.insertRange<const int>(0, {});
    CHECK(z.isEmpty());
}

TEST_CASE("Array eraseRange") {
    Array<int> x = {1, 2, 3, 4, 5};
    x.eraseRange(0, 0);
    CHECK(x == Array {1, 2, 3, 4, 5});
    x.eraseRange(0, 1);
    CHECK(x == Array {2, 3, 4, 5});
    x.eraseRange(3, 1);
    CHECK(x == Array {2, 3, 4});
    x.eraseRange(0, 3);
    CHECK(x.isEmpty());
    x = {1, 2, 3, 4, 5};
    x.eraseRange(0, 5);
    CHECK(x.isEmpty());
    x = {1, 2, 3, 4, 5};
    x.eraseRange(0, 5);
    CHECK(x.isEmpty());
    x = {1, 2, 3, 4, 5};
    x.eraseRange(5, 0);
    CHECK(x == Array {1, 2, 3, 4, 5});
    CHECK_ASSERT(x.eraseRange(5, 1));
    CHECK(x == Array {1, 2, 3, 4, 5});
    CHECK_ASSERT(x.eraseRange(6, 0));
    CHECK(x == Array {1, 2, 3, 4, 5});
    CHECK_ASSERT(x.eraseRange(6, 1));
    CHECK(x == Array {1, 2, 3, 4, 5});
    x.eraseRange(4, 1);
    CHECK(x == Array {1, 2, 3, 4});
    x.eraseRange(3, 1);
    CHECK(x == Array {1, 2, 3});
    x.eraseRange(2, 1);
    CHECK(x == Array {1, 2});
    x.eraseRange(1, 1);
    CHECK(x == Array {1});
    x.eraseRange(0, 1);
    CHECK(x.isEmpty());
}

TEST_CASE("Array replaceRange") {
    Array<int> x = {1, 2, 3, 4, 5};
    x.replaceRange<const int>(0, 0, {});
    CHECK(x == Array {1, 2, 3, 4, 5});
    x.replaceRange<const int>(0, 0, {5, 6, 7});
    CHECK(x == Array {5, 6, 7, 1, 2, 3, 4, 5});
    x.replaceRange<const int>(0, 3, {99});
    CHECK(x == Array {99, 1, 2, 3, 4, 5});
    x.replaceRange<const int>(0, 5, {99});
    CHECK(x == Array {99, 5});
    x.replaceRange<const int>(2, 0, {1, 2, 3, 4, 5});
    CHECK(x == Array {99, 5, 1, 2, 3, 4, 5});
    x.replaceRange<const int>(2, 2, {1, 2, 3, 4, 5});
    CHECK(x == Array {99, 5, 1, 2, 3, 4, 5, 3, 4, 5});
    x.replaceRange<const int>(2, 5, {});
    CHECK(x == Array {99, 5, 3, 4, 5});
}

TEST_CASE("Array with NoncopyableMovable") {
    Array<NoncopyableMovable> a;
    a.reserve(10);
    for ([[maybe_unused]] const int i : range(20)) {
        a.pushBack();
    }
    CHECK(a.size() == 20);
    for ([[maybe_unused]] const int i : range(20)) {
        a.popBack();
    }
    CHECK(a.size() == 0);

    const Array<NoncopyableMovable> b = std::move(a);
}

TEST_CASE("Array iteration") {
    Array<int> a;
    for (const int i : range(20)) {
        a.pushBack(i);
    }
    CHECK(a.size() == 20);
    int index = 0;
    for (auto& element : a) {
        CHECK(element == index++);
    }
    CHECK(index == 20);
    for (auto& element : iterateReverse(a)) {
        CHECK(element == --index);
    }
    CHECK(index == 0);
}

BUFF_NAMESPACE_END
