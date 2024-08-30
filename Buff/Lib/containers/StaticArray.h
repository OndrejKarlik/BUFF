#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/Optional.h"
#include "Lib/Serialization.h"
#include <array>

BUFF_NAMESPACE_BEGIN

template <typename T, int64 TSize, typename TIndex /*= int64*/>
class StaticArray {
    std::array<T, TSize> mImpl;

public:
    constexpr StaticArray() requires std::is_default_constructible_v<T>   = default;
    constexpr StaticArray() requires(!std::is_default_constructible_v<T>) = delete;

    /// TODO: remove the default constructability requirement
    constexpr StaticArray(std::initializer_list<T> list)
        requires std::copyable<T> && std::is_default_constructible_v<T> {
        BUFF_ASSERT(list.size() == mImpl.size());
        std::copy(list.begin(), list.end(), mImpl.begin());
    }

    constexpr const T& operator[](const TIndex index) const {
        assertValidIndex(int64(index));
        return mImpl[int64(index)];
    }
    constexpr T& operator[](const TIndex index) {
        assertValidIndex(int64(index));
        return mImpl[int64(index)];
    }

    constexpr auto begin() {
        return mImpl.begin();
    }
    constexpr auto end() {
        return mImpl.end();
    }

    constexpr auto begin() const {
        return mImpl.begin();
    }
    constexpr auto end() const {
        return mImpl.end();
    }

    T* data() {
        return mImpl.data();
    }
    const T* data() const {
        return mImpl.data();
    }

    static constexpr TIndex size() {
        return TIndex(TSize);
    }

    template <typename T2> // heterogeneous lookup
    constexpr Optional<int64> find(T2&& value) const requires EqualsComparable<T, T2> {
        for (int i = 0; i < TSize; ++i) {
            if (mImpl[i] == value) {
                return i;
            }
        }
        return NULL_OPTIONAL;
    }

    template <typename T2> // heterogeneous lookup
    constexpr bool contains(T2&& value) const requires EqualsComparable<T, T2> {
        return find(std::forward<T2>(value)).hasValue();
    }

    void serializeCustom(ISerializer& serializer) const requires Serializable<T> {
        serializer.serializeList(mImpl, "items");
    }
    void deserializeCustom(IDeserializer& deserializer) requires Deserializable<T> {
        deserializer.deserializeList(mImpl, "items");
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator ArrayView<T>() {
        return ArrayView<T>(mImpl.data(), size());
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator ArrayView<const T>() const {
        return ArrayView<const T>(mImpl.data(), size());
    }

    constexpr bool operator==(const StaticArray& other) const requires EqualsComparable<T> = default;

    void fill(const T& value) requires std::copyable<T> {
        std::fill(mImpl.begin(), mImpl.end(), value);
    }

private:
    void assertValidIndex(const int64 i) const {
        BUFF_ASSERT(i >= 0 && i < int64(size()));
    }
};

BUFF_NAMESPACE_END
