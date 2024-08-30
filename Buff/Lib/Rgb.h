#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/StaticArray.h"
#include "Lib/Math.h"
#include "Lib/Rgb8Bit.h"

BUFF_NAMESPACE_BEGIN

class Rgb;

// Defined later in the file, but already used in the Rgb class, so it needs to explicitly exist here
template <>
Rgb clamp(const Rgb& value, const Rgb& minimum, const Rgb& maximum);

class Rgb {
    StaticArray<float, 3> mData;

public:
    Rgb() = default;

    explicit Rgb(const float mono)
        : mData({mono, mono, mono}) {}

    Rgb(const float r, const float g, const float b)
        : mData({r, g, b}) {}

    float operator[](const int i) const {
        return mData[i];
    }
    float& operator[](const int i) {
        return mData[i];
    }

    float r() const {
        return mData[0];
    }
    float& r() {
        return mData[0];
    }
    float g() const {
        return mData[1];
    }
    float& g() {
        return mData[1];
    }
    float b() const {
        return mData[2];
    }
    float& b() {
        return mData[2];
    }

    static Rgb BLACK() {
        return Rgb(0.f);
    }
    static Rgb WHITE() {
        return Rgb(1.f);
    }

    Rgb operator*(const float factor) const {
        return Rgb(mData[0] * factor, mData[1] * factor, mData[2] * factor);
    }
    Rgb operator/(const float factor) const {
        return Rgb(mData[0] / factor, mData[1] / factor, mData[2] / factor);
    }
    Rgb& operator+=(const Rgb other) {
        for (int i = 0; i < mData.size(); ++i) {
            mData[i] += other.mData[i];
        }
        return *this;
    }
    Rgb& operator-=(const Rgb other) {
        for (int i = 0; i < mData.size(); ++i) {
            mData[i] -= other.mData[i];
        }
        return *this;
    }
    Rgb& operator/=(const Rgb other) {
        for (int i = 0; i < mData.size(); ++i) {
            mData[i] /= other.mData[i];
        }
        return *this;
    }
    Rgb& operator*=(const Rgb other) {
        for (int i = 0; i < mData.size(); ++i) {
            mData[i] *= other.mData[i];
        }
        return *this;
    }

    Rgb8Bit to8Bit() const {
        const Rgb multiplied = clamp(*this * 255.f, BLACK(), Rgb(255.f));
        return Rgb8Bit(uint8(multiplied.r()), uint8(multiplied.g()), uint8(multiplied.b()));
    }

    Rgb to8BitSRgb() const {
        Rgb result;
        for (int i = 0; i < 3; ++i) {
            result[i] = std::pow(mData[i], 1 / 2.2f);
        }
        return result;
    }
};

template <>
inline Rgb clamp(const Rgb& value, const Rgb& minimum, const Rgb& maximum) {
    Rgb result;
    for (int i = 0; i < 3; ++i) {
        BUFF_ASSERT(minimum[i] <= maximum[i]);
        result[i] = clamp(value[i], minimum[i], maximum[i]);
    }
    return result;
}

BUFF_NAMESPACE_END
