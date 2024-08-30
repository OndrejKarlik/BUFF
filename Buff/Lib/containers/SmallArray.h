#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/containers/StaticStack.h"
#include "Lib/Optional.h"
#include "Lib/Variant.h"

BUFF_NAMESPACE_BEGIN

template <typename T, int TSmallSize>
class SmallArray {
    Variant<StaticStack<T, TSmallSize>, Array<T>> mData;

public:
    // =======================================================================================================
    // Constructors, basic state
    // =======================================================================================================

    int64 size() const {
        return mData.visit([&](auto& array) { return int64(array.size()); });
    }
    bool isEmpty() const {
        return mData.visit([&](auto& array) { return array.isEmpty(); });
    }
    bool notEmpty() const {
        return !isEmpty();
    }

    // =======================================================================================================
    // Front, back, element access, iterators
    // =======================================================================================================

    Iterator<T> begin() {
        return mData.visit([&](auto& array) -> Iterator<T> { return array.begin(); });
    }
    Iterator<T> end() {
        return mData.visit([&](auto& array) -> Iterator<T> { return array.end(); });
    }
    Iterator<const T> begin() const {
        return mData.visit([&](const auto& array) -> Iterator<const T> { return array.begin(); });
    }
    Iterator<const T> end() const {
        return mData.visit([&](const auto& array) -> Iterator<const T> { return array.end(); });
    }

    T& front() {
        return mData.visit([&](auto&& array) -> T& { return array.front(); });
    }
    const T& front() const {
        return mData.visit([&](auto&& array) -> const T& { return array.front(); });
    }
    T& back() {
        return mData.visit([&](auto&& array) -> T& { return array.back(); });
    }
    const T& back() const {
        return mData.visit([&](auto&& array) -> const T& { return array.back(); });
    }

    const T& operator[](const int64 index) const {
        return mData.visit(
            [&](auto& array) -> const T& { return array[safeIntegerCast<decltype(array.size())>(index)]; });
    }
    T& operator[](const int64 index) {
        return mData.visit(
            [&](auto& array) -> T& { return array[safeIntegerCast<decltype(array.size())>(index)]; });
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator ArrayView<T>() {
        return mData.visit([&](auto& array) { return ArrayView<T>(array); });
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    constexpr operator ArrayView<const T>() const {
        return mData.visit([&](auto& array) { return ArrayView<const T>(array); });
    }

    constexpr ArrayView<const T> getSub(const int64           start,
                                        const Optional<int64> count = NULL_OPTIONAL) const {
        return ArrayView<const T>(*this).getSub(start, count);
    }
    constexpr ArrayView<T> getSub(const int64 start, const Optional<int64> count = NULL_OPTIONAL) {
        return ArrayView<T>(*this).getSub(start, count);
    }

    // =======================================================================================================
    // Search
    // =======================================================================================================

    // =======================================================================================================
    // Modifying end
    // =======================================================================================================

    void pushBack(const T& value) requires std::copyable<T> {
        ensureCanGrow(size() + 1);
        mData.visit([&](auto& array) { array.pushBack(value); });
    }
    void pushBack(T&& value = {}) {
        ensureCanGrow(size() + 1);
        mData.visit([&](auto& array) { array.pushBack(std::move(value)); });
    }

    template <typename... TConstructorArgs>
    void emplaceBack(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        ensureCanGrow(size() + 1);
        mData.visit([&](auto& array) { array.emplaceBack(std::forward<TConstructorArgs>(args)...); });
    }

    void popBack() {
        mData.visit([](auto& array) { array.popBack(); });
    }

    // =======================================================================================================
    // Misc modifications
    // =======================================================================================================

    void clear() {
        mData = {};
    }

    void reserve(const int64 count) {
        ensureCanGrow(count);
    }

    // =======================================================================================================
    // Misc
    // =======================================================================================================

private:
    void ensureCanGrow(const int64 totalCapacity) {
        if (totalCapacity > TSmallSize) {
            if (auto* smallStack = mData.template tryGet<StaticStack<T, TSmallSize>>()) {
                Array<T> copy;
                copy.reserve(totalCapacity);
                for (int i = 0; i < smallStack->size(); ++i) {
                    copy.pushBack(std::move((*smallStack)[i]));
                }
                mData = std::move(copy);
            }
        }
    }
};

BUFF_NAMESPACE_END
