#include "Lib/Optional.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class Optional<int>;
template class Optional<float>;
template class Optional<String>;
template class Optional<String*>;
template class Optional<NoncopyableMovable>;

TEST_CASE("Optional <=>") {
    Optional      low  = 10;
    Optional      high = 500;
    Optional<int> empty;

    CHECK(empty == empty);
    CHECK(low == low);

    CHECK(low != empty);
    CHECK(low != high);
    CHECK(empty != low);
    CHECK(empty != high);

    CHECK_FALSE(empty < empty);
    CHECK(low < high);
    // CHECK(low <= high);
    // CHECK_FALSE(low > high);
    // CHECK_FALSE(low >= high);
}

BUFF_NAMESPACE_END
