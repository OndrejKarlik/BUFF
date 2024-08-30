#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include <memory>

BUFF_NAMESPACE_BEGIN

template <typename T>
class SharedPtr {
    template <typename T2>
    friend class SharedPtr;

    template <typename T2>
    friend class WeakPtr;

    std::shared_ptr<T> mImpl;

public:
    SharedPtr() = default;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    SharedPtr(std::nullptr_t) {}

    explicit SharedPtr(T* resource)
        : mImpl(resource) {}

    template <typename TDeleter>
    explicit SharedPtr(T* resource, TDeleter deleter)
        : mImpl(resource, std::move(deleter)) {}

    T* operator->() const {
        BUFF_ASSERT(mImpl);
        return mImpl.get();
    }

    void deleteResource() {
        mImpl.reset();
    }

    T& operator*() const {
        BUFF_ASSERT(mImpl);
        return *mImpl;
    }

    /// Can return nullptr, does not throw when this is nullptr
    T* get() const {
        return mImpl.get();
    }

    template <typename T2>
    operator SharedPtr<T2>() const requires std::derived_from<T, T2> {
        SharedPtr<T2> converted;
        converted.mImpl = mImpl;
        return converted;
    }

    operator SharedPtr<const T>() const {
        SharedPtr<const T> converted;
        converted.mImpl = mImpl;
        return converted;
    }

    bool operator==(const SharedPtr& other) const = default;
    bool operator<(const SharedPtr& other) const  = default;

    // ReSharper disable once CppNonExplicitConversionOperator
    operator bool() const {
        return bool(mImpl);
    }
};

template <typename T>
class WeakPtr {
    std::weak_ptr<T> mImpl;

public:
    WeakPtr() = default;

    explicit WeakPtr(const SharedPtr<T>& sharedPtr)
        : mImpl(sharedPtr.mImpl) {}

    SharedPtr<T> lock() const {
        SharedPtr<T> result;
        result.mImpl = mImpl.lock();
        return result;
    }

    bool isExpired() const {
        return mImpl.expired();
    }
};

template <typename T, typename... TConstructorArgs>
SharedPtr<T> makeSharedPtr(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
    return SharedPtr<T>(new T(std::forward<TConstructorArgs>(args)...));
}

BUFF_NAMESPACE_END
