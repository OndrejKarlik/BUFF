#include "Lib/PoolAllocator.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class PoolAllocator<int>;
template class PoolAllocator<float>;
template class PoolAllocator<String>;
template class PoolAllocator<String*>;
template class PoolAllocator<NoncopyableMovable>;

TEST_CASE("PoolAllocator hello world") {
    PoolAllocator<String> a(3);

    Array<String*> constructed;
    constructed.pushBack(a.construct("1"));
    CHECK(*constructed.back() == "1");
    constructed.pushBack(a.construct("2"));
    CHECK(*constructed[0] == "1");
    CHECK(*constructed.back() == "2");
    constructed.pushBack(a.construct("3"));
    CHECK(*constructed.back() == "3");
    constructed.pushBack(a.construct("hello"));
    CHECK(*constructed.back() == "hello");
    constructed.pushBack(a.construct("world"));
    CHECK(*constructed.back() == "world");
    constructed.pushBack(a.construct("!"));
    CHECK(*constructed.back() == "!");
    constructed.pushBack(a.construct("last"));
    CHECK(*constructed.back() == "last");
    CHECK(*constructed[0] == "1");
    CHECK(*constructed[1] == "2");
    for (auto& i : constructed) {
        a.destruct(i);
    }
}

TEST_CASE("Pool allocator reuse") {
    PoolAllocator<String> a(3);
    const String*         ptr1 = a.construct("aaa");
    a.destruct(ptr1);
    const String* ptr2 = a.construct("aaa");
    BUFF_ASSERT(ptr1 == ptr2);
    a.destruct(ptr2);
}

BUFF_NAMESPACE_END
