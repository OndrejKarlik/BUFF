#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Serialization.h"
#include "Lib/String.h"
#include <memory>

BUFF_NAMESPACE_BEGIN

class AllocateDefaultConstructedTag {};
inline constexpr AllocateDefaultConstructedTag ALLOCATE_DEFAULT_CONSTRUCTED {};

template <typename T>
class AutoPtr : public NoncopyableMovable {
    template <typename T2>
    friend class AutoPtr;

    std::unique_ptr<T> mImpl;

public:
    AutoPtr() = default;

    AutoPtr(const AllocateDefaultConstructedTag&)
        : mImpl(new T {}) {}

    explicit AutoPtr(T* resource)
        : mImpl(resource) {}

    // ReSharper disable once CppNonExplicitConvertingConstructor
    template <typename TDerived>
    AutoPtr(AutoPtr<TDerived> other) requires std::derived_from<TDerived, T>
        : mImpl(std::move(other.mImpl)) {}

    T* operator->() const {
        BUFF_ASSERT(mImpl);
        return mImpl.get();
    }

    void deleteResource() {
        mImpl.reset();
    }

    std::add_lvalue_reference_t<T> operator*() const {
        BUFF_ASSERT(mImpl);
        return *mImpl;
    }

    /// Can return nullptr, does not throw when this is nullptr
    T* get() const {
        return mImpl.get();
    }

    friend bool operator==(const AutoPtr& x, const T* y) {
        return x.get() == y;
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator bool() const {
        return bool(mImpl);
    }

    /// Cannot resolve sharing of references!!!
    void serializeCustom(ISerializer& serializer) const
        requires Serializable<T> || std::is_base_of_v<Polymorphic, T> {
        serializer.serialize(bool(mImpl), "hasValue");
        if (bool(mImpl)) {
            if constexpr (std::is_base_of_v<Polymorphic, T>) {
                [[maybe_unused]] const T* ptr       = mImpl.get();
                const char*               className = typeid(*ptr).name();
                serializer.serialize(String(className), "className");
                Detail::serializePolymorphicClass(className, *mImpl, serializer);
            } else {
                serializer.serialize(*mImpl, "nonPolymorphicValue");
            }
        }
    }
    void deserializeCustom(IDeserializer& deserializer)
        requires Deserializable<T> || std::is_base_of_v<Polymorphic, T> {
        bool hasValue;
        deserializer.deserialize(hasValue, "hasValue");
        if (hasValue) {
            if constexpr (std::is_base_of_v<Polymorphic, T>) {
                String className;
                deserializer.deserialize(className, "className");
                Polymorphic* polymorphic =
                    Detail::instantiatePolymorphicClass(className.asCString(), deserializer);
                BUFF_ASSERT(dynamic_cast<T*>(polymorphic));
                mImpl.reset(static_cast<T*>(polymorphic));
            } else {
                mImpl = std::make_unique<T>();
                deserializer.deserialize(*mImpl, "nonPolymorphicValue");
            }
        } else {
            mImpl = nullptr;
        }
    }
};

template <typename T, typename... TConstructorArgs>
AutoPtr<T> makeAutoPtr(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
    return AutoPtr<T>(new T(std::forward<TConstructorArgs>(args)...));
}

BUFF_NAMESPACE_END
