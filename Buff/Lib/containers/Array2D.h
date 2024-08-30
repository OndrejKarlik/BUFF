#pragma once
#include "Lib/containers/Array.h"
#include "Lib/Pixel.h"
#include "Lib/Serialization.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class Array2D {
    Array<T> mImpl;
    Pixel    mSize;

public:
    Array2D()
        : mSize(0, 0) {}
    Array2D(const int width, const int height) {
        resize(width, height);
    }
    explicit Array2D(const Pixel size) {
        resize(size);
    }

    void resize(const int width, const int height) {
        mSize = Pixel(width, height);
        mImpl.resize(getPixelCount());
    }
    void resize(const Pixel size) {
        resize(size.x, size.y);
    }

    int64 getPixelCount() const {
        return mSize.x * int64(mSize.y);
    }

    const T& operator()(const int x, const int y) const {
        return mImpl[map(x, y)];
    }
    T& operator()(const int x, const int y) {
        return mImpl[map(x, y)];
    }

    const T& operator[](const Pixel& pos) const {
        return mImpl[map(pos.x, pos.y)];
    }
    // const T& operator[](const int x, const int y) const { // Not supported in MSVC yet...
    //     return mImpl[map(x, y)];
    // }
    T& operator[](const Pixel& pos) {
        return mImpl[map(pos.x, pos.y)];
    }
    // T& operator[](const int x, const int y) { // Not supported in MSVC yet...
    //     return mImpl[map(x, y)];
    // }

    const T& getNthPixel(const int64 index) const {
        return mImpl[index];
    }
    T& getNthPixel(const int64 index) {
        return mImpl[index];
    }

    Pixel size() const {
        return mSize;
    }

    T* data() {
        return mImpl.data();
    }

    const T* data() const {
        return mImpl.data();
    }

    auto begin() const {
        return mImpl.begin();
    }
    auto begin() {
        return mImpl.begin();
    }
    auto end() const {
        return mImpl.end();
    }
    auto end() {
        return mImpl.end();
    }

    void fill(const T& value) requires std::copyable<T> {
        mImpl.fill(value);
    }

    void serializeCustom(ISerializer& serializer) const requires Serializable<T> {
        serializer.serialize(mSize, "size");
        serializer.serializeList(mImpl, "data");
    }
    void deserializeCustom(IDeserializer& deserializer) requires Deserializable<T> {
        deserializer.deserialize(mSize, "size");
        mImpl.resize(mSize.getPixelCount());
        deserializer.deserializeList(mImpl, "data");
    }

private:
    int64 map(const int x, const int y) const {
        BUFF_ASSERT(x >= 0 && y >= 0 && x < mSize.x && y < mSize.y);
        return x + y * int64(mSize.x);
    }
};

BUFF_NAMESPACE_END
