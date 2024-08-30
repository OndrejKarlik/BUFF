#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include "Lib/containers/Iterator.h"
#include "Lib/Optional.h"

BUFF_NAMESPACE_BEGIN

class StringView;
template <typename T, int64 TSize, typename TIndex = int64>
class StaticArray;
template <typename T, int TSmallSize>
class SmallArray;
template <NonConst T>
class Array;

template <typename T>
class ArrayView {
    T*    mData = nullptr;
    int64 mSize = 0;

public:
    constexpr ArrayView() = default;
    constexpr ArrayView(T* data, const int64 size)
        : mData(data)
        , mSize(size) {
        BUFF_ASSERT(size >= 0, size);
    }
    constexpr ArrayView(std::initializer_list<T> list) requires std::is_const_v<T>
        : mData(&*list.begin())
        , mSize(list.size()) {}

    constexpr int64 size() const {
        return mSize;
    }
    constexpr bool isEmpty() const {
        return size() == 0;
    }
    constexpr bool notEmpty() const {
        return size() != 0;
    }

    constexpr T& operator[](const int64 index) const {
        BUFF_ASSERT(index >= 0 && index < mSize);
        return mData[index];
    }

    constexpr Iterator<T> begin() const {
        if constexpr (BUFF_DEBUG) {
            return {mData, mData, mData + mSize};
        } else {
            return Iterator {mData};
        }
    }
    constexpr Iterator<T> end() const {
        if constexpr (BUFF_DEBUG) {
            return {mData + mSize, mData, mData + mSize};
        } else {
            return Iterator {mData + mSize};
        }
    }

    constexpr T& front() const {
        return *mData;
    }

    constexpr T& back() const {
        return mData[mSize - 1];
    }

    constexpr bool contains(const T& value) const requires EqualsComparable<T> {
        return std::find(mData, mData + mSize, value) != mData + mSize;
    }

    constexpr T* data() const {
        return mData;
    }

    constexpr bool operator==(const ArrayView& other) const requires EqualsComparable<T> {
        if (size() != other.size()) {
            return false;
        }
        for (int i = 0; i < size(); ++i) {
            if ((*this)[i] != other[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr operator ArrayView<const T>() const {
        return ArrayView<const T> {mData, mSize};
    }

    constexpr ArrayView<const std::byte> asBytes() const {
        return {reinterpret_cast<const std::byte*>(mData), int64(mSize * sizeof(T))};
    }

    constexpr ArrayView getSub(const int64 start, const Optional<int64> count = NULL_OPTIONAL) const {
        BUFF_ASSERT(start >= 0 && start <= mSize);
        if (count) {
            BUFF_ASSERT(*count >= 0 && start + *count <= mSize);
            return {mData + start, *count};
        } else {
            return {mData + start, mSize - start};
        }
    }

    void serializeCustom(ISerializer& serializer) const requires Serializable<T> {
        serializer.serialize(size(), "size");
        serializer.serializeList(*this, "items");
    }
    // NO deserialization because we cannot resize the ArrayView. Serialize is layout-compatible with Array<T>
};

template <SerializableWithStdStreams T>
std::ostream& operator<<(std::ostream& os, const ArrayView<T>& value) {
    os << "[" << value.size() << "] ";
    for (auto& i : value) {
        os << "{" << i << "}, ";
    }
    return os;
}

// Deduction guide to create ArrayView from other containers

// Deduction guides are wrongly parsed by Resharper here, so we need to disable wrong naming warning:
// ReSharper disable CppInconsistentNaming
template <typename T, int64 TSize, typename TIndex = int64>
ArrayView(const StaticArray<T, TSize, TIndex>&) -> ArrayView<const T>;
template <typename T, int64 TSize, typename TIndex = int64>
ArrayView(StaticArray<T, TSize, TIndex>&) -> ArrayView<T>;

template <typename T, int TSmallSize>
ArrayView(const SmallArray<T, TSmallSize>&) -> ArrayView<const T>;
template <typename T, int TSmallSize>
ArrayView(SmallArray<T, TSmallSize>&) -> ArrayView<T>;

template <typename T>
ArrayView(const Array<T>&) -> ArrayView<const T>;
template <typename T>
ArrayView(Array<T>&) -> ArrayView<T>;

ArrayView(const StringView&) -> ArrayView<const char>;
// ReSharper restore CppInconsistentNaming

BUFF_NAMESPACE_END
