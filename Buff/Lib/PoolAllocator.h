#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/StableArray.h"
#include "Lib/containers/StaticArray.h"
#include "Lib/Math.h"
#include "Lib/Utils.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class PoolAllocator {
    StableArray<Uninitialized<T>> mPool;
    Array<T*>                     mFreeList;

public:
    explicit PoolAllocator(const int granularity)
        : mPool({.granularity = granularity}) {}

    ~PoolAllocator() {
        BUFF_ASSERT(mPool.size() == mFreeList.size(), mPool.size(), mFreeList.size());
    }

    template <typename... TConstructorArgs>
    T* construct(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        if (mFreeList.notEmpty()) {
            return new (mFreeList.popBack()) T(std::forward<TConstructorArgs>(args)...);
        } else {
            mPool.pushBack();
            Uninitialized<T>* back = &mPool.back();
            return back->construct(std::forward<TConstructorArgs>(args)...);
        }
    }

    void destruct(const T* object) {
        object->~T();
        mFreeList.pushBack(const_cast<T*>(object));
    }
};

template <typename T>
class PoolAllocatedSharedPtr {
public:
    struct Record {
        T     value;
        int64 referenceCount;

        template <typename... TConstructorArgs>
        explicit Record(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...>
            : value(std::forward<TConstructorArgs>(args)...)
            , referenceCount(1) {}
    };

private:
    PoolAllocator<Record>* mAllocator;
    Record*                mPtr;

public:
    PoolAllocatedSharedPtr()
        : mAllocator(nullptr)
        , mPtr(nullptr) {}

    template <typename... TConstructorArgs>
    explicit PoolAllocatedSharedPtr(PoolAllocator<Record>& allocator, TConstructorArgs&&... args)
        requires ConstructibleFrom<T, TConstructorArgs...>
        : mAllocator(&allocator)
        , mPtr(allocator.construct(std::forward<TConstructorArgs>(args)...)) {}

    ~PoolAllocatedSharedPtr() {
        dealloc();
    }
    PoolAllocatedSharedPtr(const PoolAllocatedSharedPtr& other)
        : mAllocator(other.mAllocator)
        , mPtr(other.mPtr) {
        if (mPtr) {
            ++mPtr->referenceCount;
        }
    }
    PoolAllocatedSharedPtr& operator=(const PoolAllocatedSharedPtr& other) {
        dealloc();
        mAllocator = other.mAllocator;
        mPtr       = other.mPtr;
        if (mPtr) {
            ++mPtr->referenceCount;
        }
        return *this;
    }
    PoolAllocatedSharedPtr(PoolAllocatedSharedPtr&& other) noexcept
        : mAllocator(other.mAllocator)
        , mPtr(other.mPtr) {
        other.mPtr = nullptr;
    }
    PoolAllocatedSharedPtr& operator=(PoolAllocatedSharedPtr&& other) noexcept {
        std::swap(mAllocator, other.mAllocator);
        std::swap(mPtr, other.mPtr);
        return *this;
    }

    operator bool() const {
        return mPtr != nullptr;
    }
    T& operator*() const {
        BUFF_ASSERT(mPtr);
        return mPtr->value;
    }
    T* operator->() const {
        BUFF_ASSERT(mPtr);
        return &mPtr->value;
    }
    bool operator==(const PoolAllocatedSharedPtr& other) const {
        BUFF_ASSERT(mAllocator == other.mAllocator);
        return mPtr == other.mPtr;
    }

private:
    void dealloc() {
        if (mPtr) {
            if (mPtr->referenceCount == 1) {
                mAllocator->destruct(mPtr);
            } else {
                --mPtr->referenceCount;
                BUFF_ASSERT(mPtr->referenceCount > 0);
            }
        }
    }
};

BUFF_NAMESPACE_END
