#include "Lib/containers/Iterator.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/containers/Array.h"

// ReSharper disable CppDiscardedPostfixOperatorResult

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class Iterator<int>;
template class Iterator<const int>;
template class Iterator<float>;
template class Iterator<const float>;
template class Iterator<String>;
template class Iterator<const String>;
template class Iterator<String*>;
template class Iterator<const String*>;
template class Iterator<NoncopyableMovable>;
template class Iterator<const NoncopyableMovable>;
template class Iterator<Noncopyable>;
template class Iterator<const Noncopyable>;

TEST_CASE("Iterator.operations") {
    Array<int>    array = {10, 20, 30, 40, 50, 60, 70};
    Iterator<int> it    = array.begin();
    CHECK(*it == 10);
    ++it;
    CHECK(*it == 20);
    it++;
    CHECK(*it == 30);
    it += 2;
    CHECK(*it == 50);
    it = it + 2;
    CHECK(*it == 70);
    it--;
    CHECK(*it == 60);
    it--;
    CHECK(*it == 50);
    it -= 3;
    CHECK(*it == 20);
    it = it + 2;
    CHECK(*it == 40);
    it = 2 + it;
    CHECK(*it == 60);
    CHECK(it[-1] == 50);
    CHECK(it[1] == 70);
    CHECK(it[0] == 60);
    CHECK(it - array.begin() == 5);
    CHECK(it == array.begin() + 5);
    CHECK(it > array.begin());
    CHECK(it < array.begin() + 6);
    CHECK_ASSERT(it[-6]);
    CHECK_ASSERT(it[2]);
}

BUFF_NAMESPACE_END
