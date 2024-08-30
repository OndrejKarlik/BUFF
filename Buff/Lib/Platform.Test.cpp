#include "Lib/Platform.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("alignedMalloc") {
    static const auto DATA = {1, 2, 4, 8, 16, 32, 64, 128};
    DOCTEST_VALUE_PARAMETERIZED_DATA(alignment, DATA);
    void* aligned = alignedMalloc(100, alignment);
    CHECK(uint64(aligned) % alignment == 0);
    std::memset(aligned, 42, 100);
    alignedFree(aligned);
}

TEST_CASE("alignedMalloc fails") {
    CHECK_ASSERT(alignedMalloc(10, 0));
    CHECK_ASSERT(alignedMalloc(0, 0));
    CHECK_ASSERT(alignedMalloc(0, 10));
    CHECK_ASSERT(alignedMalloc(-1, 10));
    CHECK_ASSERT(alignedMalloc(10, -10));
    CHECK_ASSERT(alignedMalloc(10, 3));
    CHECK_ASSERT(alignedMalloc(10, 129));
    CHECK_ASSERT(alignedMalloc(10, -8));
}

BUFF_NAMESPACE_END
