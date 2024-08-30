#include "Lib/Random.h"
#include "Lib/Bootstrap.Test.h"
#include <set>

BUFF_NAMESPACE_BEGIN

TEST_CASE("generateRandomNumber") {
    std::set<int> numbers;
    for ([[maybe_unused]] const int i : range(100)) {
        numbers.insert(generateRandomNumber<int>());
    }
    CHECK(numbers.size() == 100);
}

BUFF_NAMESPACE_END
