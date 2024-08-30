#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/containers/Iterator.h"
#include "Lib/Utils.h"

BUFF_NAMESPACE_BEGIN

template <typename T, int TMaximumSize>
class StaticStack : NoncopyableMovable /* TODO  needs manual handling */ {
    StaticArray<Uninitialized<T>, TMaximumSize> mData;
    int                                         mSize = 0;

public:
    constexpr StaticStack() = default;

    StaticStack(StaticStack&& other) noexcept {
        *this = std::move(other);
    }

    StaticStack& operator=(StaticStack&& other) noexcept {
        clear();
        mSize = other.mSize;
        for (int i = 0; i < other.size(); ++i) {
            mData[i].construct(std::move(other[i]));
        }
        return *this;
    }

    ~StaticStack() {
        clear();
    }

    constexpr int size() const {
        return mSize;
    }
    constexpr bool isEmpty() const {
        return mSize == 0;
    }
    constexpr bool notEmpty() const {
        return !isEmpty();
    }
    constexpr bool isFull() const {
        return mSize == TMaximumSize;
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator ArrayView<T>() {
        return ArrayView<T>(data(), size());
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator ArrayView<const T>() const {
        return ArrayView<const T>(data(), size());
    }

    // =======================================================================================================
    // Front, back, element access, iterators
    // =======================================================================================================

    constexpr const T& operator[](const int index) const {
        assertValidIndex(index);
        return mData[index].get();
    }
    constexpr T& operator[](const int index) {
        assertValidIndex(index);
        return mData[index].get();
    }

    T& front() {
        BUFF_ASSERT(notEmpty());
        return mData[0].get();
    }
    const T& front() const {
        BUFF_ASSERT(notEmpty());
        return mData[0].get();
    }

    T& back() {
        BUFF_ASSERT(notEmpty());
        return mData[mSize - 1].get();
    }
    const T& back() const {
        BUFF_ASSERT(notEmpty());
        return mData[mSize - 1].get();
    }

    constexpr Iterator<T> begin() {
        return getIterator<T>(*this, 0);
    }
    constexpr Iterator<T> end() {
        return getIterator<T>(*this, size());
    }
    constexpr Iterator<const T> begin() const {
        return getIterator<const T>(*this, 0);
    }
    constexpr Iterator<const T> end() const {
        return getIterator<const T>(*this, size());
    }

    T* data() {
        return &mData[0].get();
    }
    const T* data() const {
        return &mData[0].get();
    }

    // =======================================================================================================
    // Modification
    // =======================================================================================================

    void pushBack(const T& value) requires std::copyable<T> {
        BUFF_ASSERT(!isFull());
        mData[mSize].construct(value);
        ++mSize;
    }
    void pushBack(T&& value = {}) {
        BUFF_ASSERT(!isFull());
        mData[mSize].construct(std::move(value));
        ++mSize;
    }
    void popBack() {
        BUFF_ASSERT(!isEmpty());
        --mSize;
        mData[mSize].destruct();
    }

    template <typename... TConstructorArgs>
    void emplaceBack(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        BUFF_ASSERT(!isFull());
        mData[mSize].construct(std::forward<TConstructorArgs>(args)...);
        ++mSize;
    }

    void clear() {
        for (int i = 0; i < mSize; ++i) {
            mData[i].destruct();
        }
        mSize = 0;
    }

private:
    void assertValidIndex(const int64 i) const {
        BUFF_ASSERT(i >= 0 && i < size(), i, size());
    }
    template <typename TMaybeConstT, typename TMaybeConstStack>
    static Iterator<TMaybeConstT> getIterator(TMaybeConstStack& stack, [[maybe_unused]] const int64 index) {
        TMaybeConstT* base = &stack.mData[0].get();
#if BUFF_DEBUG
        return Iterator<TMaybeConstT>(base + index, base, base + stack.size());
#else
        return Iterator<TMaybeConstT>(base + index);
#endif
    }
};

BUFF_NAMESPACE_END
