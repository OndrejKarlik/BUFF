#pragma once
#include "Lib/containers/Array.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class StableArray : NoncopyableMovable {
    Array<Array<T>> mList;
    int64           mLastPtr = -1;
    int64           mBatchSize;

    template <typename TPossiblyConstArray, bool TReverse = false>
    struct Iterator {
        TPossiblyConstArray* parent;
        int64                index;
        bool                 operator==(const Iterator& other) const {
            BUFF_ASSERT(parent == other.parent);
            return index == other.index;
        }
        auto& operator*() const {
            if constexpr (TReverse) {
                return parent->operator[](parent->size() - index - 1);
            } else {
                return parent->operator[](index);
            }
        }
        void operator++() {
            ++index;
        }
    };

public:
    // =======================================================================================================
    // Constructors, basic state
    // =======================================================================================================

    struct StableArrayInitializer {
        int64 granularity;
    };
    StableArray(StableArrayInitializer initializer)
        : mBatchSize(initializer.granularity) {}

    int64 size() const {
        if (mLastPtr == -1) {
            return 0;
        } else {
            return int64(mLastPtr) * mBatchSize + mList[mLastPtr].size();
        }
    }
    bool isEmpty() const {
        return mLastPtr == -1;
    }
    bool notEmpty() const {
        return !isEmpty();
    }

    // =======================================================================================================
    // Front, back, element access, iterators
    // =======================================================================================================

    T& back() {
        BUFF_ASSERT(notEmpty());
        return mList[mLastPtr].back();
    }
    const T& back() const {
        BUFF_ASSERT(notEmpty());
        return mList[mLastPtr].back();
    }

    const T& operator[](const int64 index) const {
        assertValidIndex(index);
        const std::lldiv_t indices = std::lldiv(index, mBatchSize);
        return mList[indices.quot][indices.rem];
    }
    T& operator[](const int64 index) {
        assertValidIndex(index);
        const std::lldiv_t indices = std::lldiv(index, mBatchSize);
        return mList[indices.quot][indices.rem];
    }

    auto begin() {
        return Iterator<StableArray> {this, 0};
    }
    auto end() {
        return Iterator<StableArray> {this, size()};
    }
    auto begin() const {
        return Iterator<const StableArray> {this, 0};
    }
    auto end() const {
        return Iterator<const StableArray> {this, size()};
    }

    auto rbegin() {
        return Iterator<StableArray, true> {this, 0};
    }
    auto rend() {
        return Iterator<StableArray, true> {this, size()};
    }
    auto rbegin() const {
        return Iterator<const StableArray, true> {this, 0};
    }
    auto rend() const {
        return Iterator<const StableArray, true> {this, size()};
    }

    // =======================================================================================================
    // Search
    // =======================================================================================================

    // =======================================================================================================
    // Modifying end
    // =======================================================================================================

    void pushBack(const T& value) requires std::copyable<T> {
        ensureCapacityAtEnd();
        mList[mLastPtr].pushBack(value);
    }
    void pushBack(T&& value = {}) requires std::movable<T> {
        ensureCapacityAtEnd();
        mList[mLastPtr].pushBack(std::move(value));
    }

    template <typename... TConstructorArgs>
    void emplaceBack(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        ensureCapacityAtEnd();
        mList[mLastPtr].emplaceBack(std::forward<TConstructorArgs>(args)...);
    }

    void popBack() {
        BUFF_ASSERT(notEmpty());
        mList[mLastPtr].resize(mList[mLastPtr].size() - 1);
        if (mList[mLastPtr].isEmpty()) {
            --mLastPtr;
        }
    }

    // =======================================================================================================
    // Misc modifications
    // =======================================================================================================

    void clear() {
        for (auto& i : mList) {
            i.clear();
        }
        mLastPtr = -1;
    }

    // =======================================================================================================
    // Misc
    // =======================================================================================================

private:
    void ensureCapacityAtEnd() {
        if (mLastPtr == -1 || mList[mLastPtr].size() == mBatchSize) {
            if (mLastPtr == mList.size() - 1) {
                mList.emplaceBack();
                mList.back().reserve(mBatchSize);
            }
            ++mLastPtr;
            BUFF_ASSERT(mList[mLastPtr].isEmpty());
        }
    }
    void assertValidIndex(const int64 i) const {
        BUFF_ASSERT(i >= 0 && i < size(), i, size());
    }
};

BUFF_NAMESPACE_END
