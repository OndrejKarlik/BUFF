#pragma once
#include "Lib/Bootstrap.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class Iterator {
    T* mValue = nullptr;
#if BUFF_DEBUG
    const T* mBegin = nullptr;
    const T* mEnd   = nullptr;
#endif

public:
    // ReSharper disable CppInconsistentNaming
    using value_type        = T;
    using difference_type   = int64;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::random_access_iterator_tag;
    // ReSharper restore CppInconsistentNaming

    Iterator() = default;
#if BUFF_DEBUG
    constexpr Iterator(T* value, const T* begin, const T* end)
        : mValue(value)
        , mBegin(begin)
        , mEnd(end) {}
#else
    constexpr explicit Iterator(T* value)
        : mValue(value) {}
#endif

    constexpr Iterator& operator++() {
        ++mValue;
        return *this;
    }
    constexpr Iterator operator++(int) {
        Iterator copy = *this;
        ++mValue;
        return copy;
    }
    constexpr Iterator& operator--() {
        --mValue;
        return *this;
    }
    constexpr Iterator operator--(int) {
        Iterator copy = *this;
        --mValue;
        return copy;
    }
    friend constexpr Iterator operator+(int64 count, Iterator it) {
        it.mValue += count;
        return it;
    }
    constexpr Iterator operator+(int64 count) const {
        Iterator copy = *this;
        copy.mValue += count;
        return copy;
    }
    constexpr Iterator& operator+=(const int64 count) {
        mValue += count;
        return *this;
    }
    constexpr Iterator operator-(int64 count) const {
        Iterator copy = *this;
        copy.mValue -= count;
        return copy;
    }
    constexpr Iterator& operator-=(const int64 count) {
        mValue -= count;
        return *this;
    }
    constexpr int64 operator-(const Iterator& other) const {
        assertSameRange(other);
        return mValue - other.mValue;
    }
    constexpr bool operator==(const Iterator& other) const {
        assertSameRange(other);
        return mValue == other.mValue;
    }
    constexpr auto operator<=>(const Iterator& other) const {
        assertSameRange(other);
        return mValue <=> other.mValue;
    }
    constexpr T& operator*() const {
        assertDereferenceable();
        return *mValue;
    }
    constexpr T& operator[](const int64 index) const {
        assertDereferenceable(index);
        return mValue[index];
    }
    constexpr T* operator->() const {
        assertDereferenceable();
        return mValue;
    }

private:
    constexpr void assertSameRange([[maybe_unused]] const Iterator& other) const {
#if BUFF_DEBUG
        BUFF_ASSERT(mBegin == other.mBegin && mEnd == other.mEnd);
#endif
    }
    constexpr void assertDereferenceable([[maybe_unused]] const int64 offset = 0) const {
#if BUFF_DEBUG
        BUFF_ASSERT(mValue + offset >= mBegin && mValue + offset < mEnd);
#endif
    }
};

BUFF_NAMESPACE_END
