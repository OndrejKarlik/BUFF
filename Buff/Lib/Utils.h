#pragma once
#include "Lib/Concepts.h"
#include "Lib/containers/StaticArray.h"
#include <chrono>
#include <functional>
#include <utility>

BUFF_NAMESPACE_BEGIN

namespace Detail {

template <typename T, int TCount>
struct AnyOf {
    StaticArray<T, TCount> items;

    template <typename T2>
    constexpr bool operator==(const T2& other) const requires EqualsComparable<T, T2> {
        return items.contains(other);
    }
};

} // namespace Detail

template <typename T0, typename... TOther>
constexpr auto anyOf(T0&& first, TOther&&... rest) {
    using T                 = std::decay_t<T0>;
    constexpr int ARG_COUNT = 1 + sizeof...(TOther);
    return Detail::AnyOf<T, ARG_COUNT> {
        StaticArray<T, ARG_COUNT>({std::forward<T0>(first), std::forward<TOther>(rest)...})};
}

template <Pointer TTo, Pointer TFrom>
TTo safeStaticCast(TFrom&& from) {
    if constexpr (BUFF_DEBUG) {
        BUFF_ASSERT(dynamic_cast<TTo>(from));
    }
    return static_cast<TTo>(from);
}

template <Integer TTo, Integer TFrom>
TTo safeIntegerCast(TFrom from) {
    // std::cmp_equal does not work with chars, so we need custom implementation
    const TTo to = static_cast<TTo>(from);

    if constexpr (std::is_signed_v<TTo> && std::is_unsigned_v<TFrom>) {
        BUFF_ASSERT(static_cast<std::make_unsigned_t<TTo>>(to) == from && to >= 0);
    } else if constexpr (std::is_signed_v<TFrom> && std::is_unsigned_v<TTo>) {
        BUFF_ASSERT(static_cast<std::make_unsigned_t<TFrom>>(from) == to && from >= 0);
    } else {
        static_assert(std::is_signed_v<TTo> == std::is_signed_v<TFrom>);
        BUFF_ASSERT(to == from);
    }
    return to;
}

class Finally {
    /// Using Function<> here would create a circular dependency
    std::function<void()> mFunctor;

public:
    // ReSharper disable once CppNonExplicitConvertingConstructor
    template <typename T>
    [[nodiscard]] Finally(T&& functor) requires(!std::is_same_v<T, Finally>)
        : mFunctor(std::forward<T>(functor)) {}

    ~Finally() {
        mFunctor();
    }
};

/// Adds an exact number of bytes to the pointer. Note that this is different from just doing ptr += x,
/// because the latter adds x * sizeof(T) bytes.
template <typename T>
T* addBytesToPointer(T* input, const int64 offset) {
    std::uintptr_t ptrValue = reinterpret_cast<std::uintptr_t>(input);
    ptrValue += offset;
    return reinterpret_cast<T*>(ptrValue);
}

constexpr int orderingToInteger(const std::strong_ordering cmp) noexcept {
    BUFF_DISABLE_CLANG_WARNING_BEGIN("-Wzero-as-null-pointer-constant")
    // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
    if (cmp < 0) {
        return -1;
        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
    } else if (cmp > 0) {
        return 1;
    } else {
        return 0;
    }
    BUFF_DISABLE_CLANG_WARNING_END()
}

template <typename T>
T min(const T& x, const T& y, const T& z) {
    return std::min(x, std::min(y, z));
}

template <typename T>
int argMin(const T& x, const T& y, const T& z) {
    return x < y ? (x < z ? 0 : 2) : (y < z ? 1 : 2);
}

template <typename T>
T max(const T& x, const T& y, const T& z) {
    return std::max(x, std::max(y, z));
}

template <typename T>
int argMax(const T& x, const T& y, const T& z) {
    return x > y ? (x > z ? 0 : 2) : (y > z ? 1 : 2);
}

template <typename T>
// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
class alignas(T) Uninitialized {
    // ReSharper disable once CppUninitializedNonStaticDataMember
    std::byte mDummy[sizeof(T)];

public:
    T& get() {
        return *reinterpret_cast<T*>(&mDummy);
    }
    const T& get() const {
        return *reinterpret_cast<const T*>(&mDummy);
    }

    template <typename... TConstructorArgs>
    T* construct(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        return new (&mDummy) T(std::forward<TConstructorArgs>(args)...);
    }
    void destruct() {
        get().~T();
    }
};

template <Integer T>
auto range(const int begin, const T end) {
    struct Iterator {
        T value;
        T endValue;

        T operator*() const {
            BUFF_ASSERT(value >= 0 && value < endValue);
            return value;
        }
        Iterator& operator++() {
            ++value;
            return *this;
        }
        bool operator!=(const Iterator& other) const {
            BUFF_ASSERT(endValue == other.endValue);
            return value != other.value;
        }
    };
    struct IteratorHolder {
        T        beginVal;
        T        endVal;
        Iterator begin() const {
            return {beginVal, endVal};
        }
        Iterator end() const {
            return {endVal, endVal};
        }
    };
    return IteratorHolder {T(begin), end};
}

template <Integer T>
auto range(const T count) {
    return range<T>(0, count);
}

namespace Detail {
template <typename TIterator, typename TValue>
struct IterationValueWrapper {
    TIterator* it;

    IterationValueWrapper() = default;
    IterationValueWrapper(TIterator* it)
        : it(it) {}
    operator TValue&() {
        return *it->it;
    }
    auto* operator->() {
        return it->it.operator->();
    }
    const auto* operator->() const {
        return it->it.operator->();
    }
    TValue& value() {
        return *(it->it);
    }
    const TValue& value() const {
        return *it->it;
    }
    int64 index() const {
        return it->index();
    }
    bool isLast() const {
        return it->isLast();
    }
    bool notLast() const {
        return !it->isLast();
    }
    bool isFirst() const {
        return it->isFirst();
    }
    bool notFirst() const {
        return !it->isFirst();
    }

    template <typename T2>
    decltype(auto) operator[](T2&& index) {
        return (*it->it)[std::forward<T2>(index)];
    }
    template <typename T2>
    auto operator=(T2&& value) -> decltype(*it->it = value) {
        return (*it->it) = std::forward<T2>(value);
    }
    template <typename T2>
    bool operator==(T2&& value) const {
        return this->value() == value;
    }
};
} // namespace Detail

template <IterableContainer T>
auto iterate(T&& container) {
    static_assert(std::is_reference_v<T>);
    using UnderlyingIterator = std::decay_t<decltype(container.begin())>;

    struct Iterator {
        UnderlyingIterator it;
        T&                 container;
        using WrapperType = Detail::IterationValueWrapper<Iterator, std::decay_t<decltype(*it)>>;
        WrapperType wrapper;

        Iterator(UnderlyingIterator it, T& container)
            : it(std::move(it))
            , container(container) {
            // Cannot use member initializer list because of "this"
            wrapper = WrapperType(this); // NOLINT(cppcoreguidelines-prefer-member-initializer)
        }

        WrapperType& operator*() {
            return wrapper;
        }
        Iterator& operator++() {
            ++it;
            return *this;
        }
        bool operator!=(const Iterator& other) const {
            return it != other.it;
        }

        int64 index() const {
            return it - container.begin();
        }
        bool isLast() const {
            return it == container.end() - 1;
        }
        bool isFirst() const {
            return it == container.begin();
        }
    };
    struct IteratorHolder {
        T& container;
        IteratorHolder(T& container)
            : container(container) {}
        Iterator begin() const {
            return Iterator {container.begin(), container};
        }
        Iterator end() const {
            return Iterator {container.end(), container};
        }
    };
    return IteratorHolder {container};
}

template <IterableContainer T>
auto iterateReverse(T&& container) {
    static_assert(std::is_reference_v<T>);
    struct Adapter {
        T& container;
        Adapter(T& container)
            : container(container) {}
        auto begin() const {
            return container.rbegin();
        }
        auto end() const {
            return container.rend();
        }
    };
    return Adapter {container};
}

BUFF_NAMESPACE_END
