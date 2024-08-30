#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/StaticArray.h"

BUFF_NAMESPACE_BEGIN

class Rgb8Bit {
    StaticArray<uint8, 3> mData;

public:
    Rgb8Bit() = default;

    constexpr Rgb8Bit(const uint8 r, const uint8 g, const uint8 b)
        : mData({r, g, b}) {}

    constexpr explicit Rgb8Bit(const StaticArray<uint8, 3>& rgb)
        : mData(rgb) {}

    constexpr explicit Rgb8Bit(const uint8 mono)
        : mData({mono, mono, mono}) {}

    static constexpr Rgb8Bit fromHex(const int color) {
        BUFF_ASSERT((color & ~0xFFFFFF) == 0);
        return {uint8(color >> 16), uint8((color >> 8) & 0xFF), uint8(color & 0xFF)};
    }

    constexpr static Rgb8Bit BLACK() {
        return {0, 0, 0};
    }
    constexpr static Rgb8Bit WHITE() {
        return {255, 255, 255};
    }

    constexpr uint8 operator[](const int index) const {
        return mData[index];
    }
    constexpr uint8& operator[](const int index) {
        return mData[index];
    }
};

struct Rgba8Bit {
private:
    StaticArray<uint8, 4> mData;

public:
    Rgba8Bit() = default;

    constexpr Rgba8Bit(const uint8 r, const uint8 g, const uint8 b, const uint8 a = 255)
        : mData({r, g, b, a}) {}

    constexpr explicit Rgba8Bit(const Rgb8Bit& color, const uint8 alpha = 255)
        : mData({color[0], color[1], color[2], alpha}) {}

    constexpr explicit Rgba8Bit(const uint8 mono)
        : mData({mono, mono, mono, 255}) {}

    constexpr static Rgba8Bit BLACK() {
        return {0, 0, 0, 255};
    }
    constexpr static Rgba8Bit WHITE() {
        return {255, 255, 255, 255};
    }

    constexpr uint8 operator[](const int index) const {
        BUFF_ASSERT(index < 3);
        return mData[index];
    }
    constexpr uint8& operator[](const int index) {
        BUFF_ASSERT(index < 3);
        return mData[index];
    }
    constexpr uint8 alpha() const {
        return mData[3];
    }
    constexpr uint8& alpha() {
        return mData[3];
    }

    constexpr float grayValue() const {
        return float(mData[0] + mData[1] + mData[2]) / 3.f;
    }
    constexpr bool operator==(const Rgba8Bit& other) const = default;

    constexpr size_t getHash() const {
        return mData[0] + (mData[1] << 8) + (mData[2] << 16) + (mData[3] << 24);
    }
};

BUFF_NAMESPACE_END
