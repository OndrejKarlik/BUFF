#include "Lib/containers/StaticStack.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class StaticStack<int, 1>;
template class StaticStack<float, 2>;
template class StaticStack<String, 3>;
template class StaticStack<String*, 4>;
template class StaticStack<NoncopyableMovable, 5>;

TEST_CASE("StaticStack iteration") {
    StaticStack<int, 30> a;
    for (const int i : range(20)) {
        a.pushBack(i);
    }
    CHECK(a.size() == 20);
    int index = 0;
    for (auto& element : a) {
        CHECK(element == index++);
    }
    CHECK(index == 20);
}

BUFF_NAMESPACE_END
