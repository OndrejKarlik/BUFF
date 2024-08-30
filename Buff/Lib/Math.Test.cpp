#include "Lib/Math.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

TEST_CASE("isReal") {
    CHECK(isReal(0.f));
    CHECK(isReal(1.5f));
    CHECK(isReal(1.f / 1e10f));
    CHECK(isReal(1e20f));

    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const volatile float zero = 0.f;
    CHECK(!isReal(zero / zero));
    CHECK(!isReal(std::pow(10.f, 100.f)));

    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const volatile float bigNumber = 1e20f;
    CHECK(!isReal(bigNumber * bigNumber));

    CHECK(!isReal(std::numeric_limits<float>::quiet_NaN()));
    CHECK(!isReal(std::numeric_limits<float>::signaling_NaN()));
    CHECK(!isReal(std::numeric_limits<float>::infinity()));
    CHECK(!isReal(-std::numeric_limits<float>::infinity()));
}

TEST_CASE("isNan") {
    CHECK(!isNan(0.f));
    CHECK(!isNan(1.5f));
    CHECK(!isNan(1.f / 1e10f));
    CHECK(!isNan(1e20f));

    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const volatile float zero = 0.f;
    CHECK(isNan(zero / zero));
    CHECK(!isNan(std::pow(10.f, 100.f)));
    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const volatile float bigNumber = 1e20f;
    CHECK(!isNan(bigNumber * bigNumber));

    CHECK(isNan(std::numeric_limits<float>::quiet_NaN()));
    CHECK(isNan(std::numeric_limits<float>::signaling_NaN()));
    CHECK(!isNan(std::numeric_limits<float>::infinity()));
    CHECK(!isNan(-std::numeric_limits<float>::infinity()));
}

TEST_CASE("getNumDigits") {
    CHECK(getNumDigits(0u) == 1);
    CHECK(getNumDigits(1u) == 1);
    CHECK(getNumDigits(9u) == 1);
    CHECK(getNumDigits(10u) == 2);
    CHECK(getNumDigits(100u) == 3);
    CHECK(getNumDigits(1000u) == 4);
    CHECK(getNumDigits(10000u) == 5);
    CHECK(getNumDigits(100000000000u) == 12);
    CHECK(getNumDigits(999u) == 3);
}

TEST_CASE("bitSetCount") {
    CHECK(bitSetCount(0u) == 0);
    CHECK(bitSetCount(1u) == 1);
    CHECK(bitSetCount(2u) == 1);
    CHECK(bitSetCount(3u) == 2);
    CHECK(bitSetCount(4u) == 1);
    CHECK(bitSetCount(127) == 7);
    CHECK(bitSetCount(128) == 1);
    CHECK(bitSetCount(-1) == 32);
}

TEST_CASE("alignUp") {
    CHECK(alignUp(0u, 32) == 0);
    CHECK(alignUp(1u, 32) == 32);
    CHECK(alignUp(31u, 32) == 32);
    CHECK(alignUp(32u, 32) == 32);
    CHECK(alignUp(32000u, 32) == 32000);
    CHECK(alignUp(320001u, 32) == 320032);
    CHECK(alignUp(320031u, 32) == 320032);
    CHECK(alignUp(320032u, 32) == 320032);
    CHECK(alignUp(320032u, 32) == 320032);
}

TEST_CASE("isPowerOf2") {
    CHECK(!isPowerOf2(0));
    CHECK(isPowerOf2(1));
    CHECK(isPowerOf2(2));
    CHECK(isPowerOf2(4));
    CHECK(isPowerOf2(8));
    CHECK(isPowerOf2(64));
    CHECK(isPowerOf2(1024));
    CHECK(!isPowerOf2(3));
    CHECK(!isPowerOf2(9));
    CHECK(!isPowerOf2(1023));
    CHECK(!isPowerOf2(1025));
    CHECK(!isPowerOf2(192));
    CHECK(!isPowerOf2(96));
    CHECK(!isPowerOf2(12));
    CHECK(!isPowerOf2(1024 + 512));
}

BUFF_NAMESPACE_END
