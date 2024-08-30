#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Pixel.h"
#include "Lib/Vector.h"

BUFF_NAMESPACE_BEGIN

class BoundingBox2I {
    Pixel mTopLeft = Pixel::ZERO();
    Pixel mSize    = Pixel::ZERO();

public:
    BoundingBox2I() = default;

    /// Top Left + size specification
    BoundingBox2I(const Pixel topLeft, const Pixel size)
        : mTopLeft(topLeft)
        , mSize(size) {
        BUFF_ASSERT(mSize.x > 0 && mSize.y > 0);
    }

    Pixel getTopLeft() const {
        return mTopLeft;
    }
    void setTopLeft(const Pixel topLeft) {
        mTopLeft = topLeft;
    }
    Pixel getSize() const {
        return mSize;
    }
    Pixel getPastBottomRight() const {
        return mTopLeft + mSize;
    }

    bool contains(const Pixel pixel) const {
        const Pixel pastBottomRight = getPastBottomRight();
        return pixel.x >= mTopLeft.x && pixel.y >= mTopLeft.y && pixel.x < pastBottomRight.x &&
               pixel.y < pastBottomRight.y;
    }
};

class BoundingBox2 {
    Vector2 mTopLeft = Vector2::ZERO();
    Vector2 mSize    = Vector2::ZERO();

public:
    BoundingBox2() = default;

    /// Top Left + size specification
    BoundingBox2(const Vector2 topLeft, const Vector2 size)
        : mTopLeft(topLeft)
        , mSize(size) {
        BUFF_ASSERT(mSize.x > 0 && mSize.y > 0);
    }

    Vector2 getTopLeft() const {
        return mTopLeft;
    }
    Vector2 getSize() const {
        return mSize;
    }
    Vector2 getPastBottomRight() const {
        return mTopLeft + mSize;
    }

    bool contains(const Vector2 pixel) const {
        const Vector2 pastBottomRight = getPastBottomRight();
        return pixel.x >= mTopLeft.x && pixel.y >= mTopLeft.y && pixel.x < pastBottomRight.x &&
               pixel.y < pastBottomRight.y;
    }
};

BUFF_NAMESPACE_END
