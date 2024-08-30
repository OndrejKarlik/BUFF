#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include <cmath>

BUFF_NAMESPACE_BEGIN

inline constexpr float PI = 3.14159265358979323846f;

inline bool isReal(const float in) {
    return std::isfinite(in);
}
inline bool isReal(const double in) {
    return std::isfinite(in);
}

inline bool isNan(const float in) {
    return std::isnan(in);
}
inline bool isNan(const double in) {
    return std::isnan(in);
}

template <typename T>
T clamp(const T& value, const T& minimum, const T& maximum) {
    BUFF_ASSERT(minimum <= maximum);
    return max(minimum, min(value, maximum));
}

inline int reinterpretInt(const float x) {
    return std::bit_cast<int>(x);
}

/// Returns number of digits in the given number.
template <UnsignedInteger T>
int getNumDigits(T input) {
    return int(max(0.f, std::log10(float(input)))) + 1;
}

template <Integer T>
int bitSetCount(const T input) {
    int count = 0;
    for (int i = 0; i < sizeof(T) * 8; ++i) {
        count += input & (1 << i) ? 1 : 0;
    }
    return count;
}

/// Works in bytes
template <Integer T>
T alignUp(T input, int alignment) {
    BUFF_ASSERT(input >= 0 && alignment > 0 && bitSetCount(alignment) == 1);
    return (input + T(alignment - 1)) & T(-alignment);
}

/// Works only for non-negative numbers!
template <Integer T>
bool isPowerOf2(const T input) {
    BUFF_ASSERT(input >= 0);
    // emscripten does not have std::has_single_bit
    return input && !(input & (input - 1));
}

BUFF_NAMESPACE_END
