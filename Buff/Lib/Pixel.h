#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Math.h"

BUFF_NAMESPACE_BEGIN
struct Vector2;

struct Pixel {
    int x = 0;
    int y = 0;

    constexpr Pixel() = default;
    constexpr explicit Pixel(const int value)
        : x(value)
        , y(value) {}
    constexpr Pixel(const int x, const int y)
        : x(x)
        , y(y) {}

    constexpr static Pixel ZERO() {
        return Pixel(0);
    }
    constexpr static Pixel ONE() {
        return Pixel(1);
    }

    constexpr bool operator==(const Pixel& other) const = default;

    constexpr int64 getPixelCount() const {
        return int64(x) * y;
    }

    constexpr void enumerateStructMembers(auto&& functor) {
        functor(x, "x");
        functor(y, "y");
        BUFF_ASSERT_SIZEOF(4 + 4);
    }
    // TODO: remove with deducing this
    constexpr void enumerateStructMembers(auto&& functor) const {
        functor(x, "x");
        functor(y, "y");
        BUFF_ASSERT_SIZEOF(4 + 4);
    }
};

constexpr Pixel operator+(const Pixel& a, const Pixel& b) {
    return Pixel(a.x + b.x, a.y + b.y);
}
constexpr Pixel& operator+=(Pixel& a, const Pixel& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}
constexpr Pixel operator-(const Pixel& a, const Pixel& b) {
    return Pixel(a.x - b.x, a.y - b.y);
}
constexpr Pixel& operator-=(Pixel& a, const Pixel& b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}
constexpr Pixel operator%(const Pixel& a, const Pixel& b) {
    return Pixel(a.x % b.x, a.y % b.y);
}
constexpr Pixel operator*(const Pixel& pixel, const int factor) {
    return Pixel(pixel.x * factor, pixel.y * factor);
}
constexpr Pixel operator/(const Pixel& pixel, const int factor) {
    return Pixel(pixel.x / factor, pixel.y / factor);
}

inline Pixel clamp(const Pixel value, const Pixel minimum, const Pixel maximum) {
    return {clamp(value.x, minimum.x, maximum.x), clamp(value.y, minimum.y, maximum.y)};
}

Pixel   toPixel(Vector2 input);
Vector2 toVector2(Pixel input);

BUFF_NAMESPACE_END
