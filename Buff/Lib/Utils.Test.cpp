#include "Lib/Utils.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

static_assert(IsEqualsComparable<int, Detail::AnyOf<int, 3>>);

TEST_CASE("anyOf") {

    CHECK(1 == anyOf(0, 1, 2, 3));
    CHECK(6 != anyOf(0, 1, 2, 3));
}

TEST_CASE("safeIntegerCast") {
    CHECK(safeIntegerCast<int>(char(1)) == 1);

    CHECK(safeIntegerCast<unsigned char>(255) == 255);
    CHECK(safeIntegerCast<unsigned char>(0) == 0);
    CHECK_ASSERT(safeIntegerCast<unsigned char>(256));
    CHECK_ASSERT(safeIntegerCast<unsigned char>(-1));

    CHECK_ASSERT(safeIntegerCast<signed char>(128));
    CHECK_ASSERT(safeIntegerCast<signed char>(-129));

    CHECK_ASSERT(safeIntegerCast<unsigned>(-1));
    CHECK_ASSERT(safeIntegerCast<int>(UINT_MAX));
}

TEST_CASE("min") {
    // 3-argument min test
    CHECK(min(1, 2, 3) == 1);
    CHECK(min(2, 1, 3) == 1);
    CHECK(min(3, 2, 1) == 1);
    CHECK(min(1, 1, 1) == 1);
    CHECK(min(1, 1, 2) == 1);
    CHECK(min(1, 2, 1) == 1);
    CHECK(min(2, 1, 1) == 1);
    CHECK(min(1, 2, 2) == 1);
    CHECK(min(2, 1, 2) == 1);
    CHECK(min(2, 2, 1) == 1);
    CHECK(min(2, 2, 2) == 2);
}
TEST_CASE("argMin") {
    // 3-argument argMin test
    CHECK(argMin(100, 200, 300) == 0);
    CHECK(argMin(200, 100, 300) == 1);
    CHECK(argMin(300, 200, 100) == 2);
    CHECK(argMin(100, 200, 200) == 0);
    CHECK(argMin(200, 100, 200) == 1);
    CHECK(argMin(200, 200, 100) == 2);
}
TEST_CASE("argMax") {
    // 3-argument argMax test
    CHECK(argMax(100, 200, 300) == 2);
    CHECK(argMax(200, 100, 300) == 2);
    CHECK(argMax(300, 200, 100) == 0);
    CHECK(argMax(100, 100, 200) == 2);
    CHECK(argMax(100, 200, 100) == 1);
    CHECK(argMax(200, 100, 100) == 0);
}

TEST_CASE("max") {
    // 3-argument max test
    CHECK(max(1, 2, 3) == 3);
    CHECK(max(2, 1, 3) == 3);
    CHECK(max(3, 2, 1) == 3);
    CHECK(max(1, 1, 1) == 1);
    CHECK(max(1, 1, 2) == 2);
    CHECK(max(1, 2, 1) == 2);
    CHECK(max(2, 1, 1) == 2);
    CHECK(max(1, 2, 2) == 2);
    CHECK(max(2, 1, 2) == 2);
    CHECK(max(2, 2, 1) == 2);
    CHECK(max(2, 2, 2) == 2);
}

BUFF_NAMESPACE_END
