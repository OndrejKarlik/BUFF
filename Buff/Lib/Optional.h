#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include "Lib/Serialization.h"
#include <optional>

BUFF_NAMESPACE_BEGIN

struct NullOptional {};

inline constexpr NullOptional NULL_OPTIONAL;

template <typename T>
class Optional {
    std::optional<T> mImpl;

public:
    constexpr Optional() = default;
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr Optional(NullOptional BUFF_UNUSED(dummy))
        : mImpl(std::nullopt) {}
    constexpr Optional(const Optional& value)                = default;
    constexpr Optional(Optional&& value) noexcept            = default;
    constexpr Optional& operator=(const Optional& value)     = default;
    constexpr Optional& operator=(Optional&& value) noexcept = default;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr Optional(const T& value) requires std::copyable<T>
        : mImpl(value) {}

    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr Optional(T&& value)
        : mImpl(std::move(value)) {}

    template <typename T2>
    constexpr Optional& operator=(T2 value) requires std::assignable_from<T, T2> {
        mImpl = std::move(value);
        return *this;
    }

    void deleteResource() {
        mImpl.reset();
    }

    [[nodiscard]] constexpr explicit operator bool() const {
        return mImpl.has_value();
    }

    [[nodiscard]] constexpr const T& operator*() const {
        BUFF_ASSERT(bool(mImpl));
        return *mImpl;
    }
    [[nodiscard]] constexpr T& operator*() {
        BUFF_ASSERT(bool(mImpl));
        return *mImpl;
    }
    constexpr const T* operator->() const {
        BUFF_ASSERT(bool(mImpl));
        return &*mImpl;
    }
    constexpr T* operator->() {
        BUFF_ASSERT(bool(mImpl));
        return &*mImpl;
    }

    template <typename... TConstructorArgs>
    constexpr void emplace(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        mImpl.emplace(std::forward<TConstructorArgs>(args)...);
    }

    [[nodiscard]] constexpr bool hasValue() const {
        return mImpl.has_value();
    }
    [[nodiscard]] constexpr const T& valueOr(const T& alternative) const {
        return mImpl ? *mImpl : alternative;
    }

    template <typename T2>
    [[nodiscard]] constexpr bool operator<(const Optional<T2>& y) const requires LessThanComparable<T, T2> {
        return mImpl < y.mImpl;
    }
    template <typename T2>
    [[nodiscard]] constexpr bool operator==(const Optional<T2>& y) const requires EqualsComparable<T, T2> {
        return mImpl == y.mImpl;
    }
    template <typename T2>
    [[nodiscard]] constexpr bool operator==(const T2& y) const requires EqualsComparable<T, T2> {
        return mImpl == y;
    }
    [[nodiscard]] constexpr bool operator==(const NullOptional& BUFF_UNUSED(y)) const {
        return !hasValue();
    }

    static auto threeWayCompare(const Optional& a, const Optional& b) requires std::three_way_comparable<T> {
        using Result              = std::compare_three_way_result_t<T>;
        const Result hasValueComp = a.hasValue() <=> b.hasValue();
        if (hasValueComp != 0) {
            return hasValueComp;
        } else if (!a.hasValue()) {
            return Result(std::strong_ordering::equal);
        } else {
            return *a <=> *b;
        }
    }

    void serializeCustom(ISerializer& serializer) const requires Serializable<T> {
        serializer.serialize(hasValue(), "hasValue");
        if (hasValue()) {
            serializer.serialize(**this, "value");
        }
    }
    void deserializeCustom(IDeserializer& deserializer) requires Deserializable<T> {
        bool willHaveValue;
        deserializer.deserialize(willHaveValue, "hasValue");
        if (willHaveValue) {
            emplace();
            deserializer.deserialize(**this, "value");
        } else {
            deleteResource();
        }
    }
};

template <SerializableWithStdStreams T>
std::ostream& operator<<(std::ostream& os, const Optional<T>& value) {
    if (value) {
        os << "Optional(" << *value << ")";
    } else {
        os << "Optional()";
    }
    return os;
}

BUFF_NAMESPACE_END
