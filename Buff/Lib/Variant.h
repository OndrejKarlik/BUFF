#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include <variant>

BUFF_NAMESPACE_BEGIN

namespace Detail {
template <typename... TFunctors>
struct VariantVisitor : TFunctors... {
    VariantVisitor() = delete;
    VariantVisitor(TFunctors... functors)
        : TFunctors(functors)... {}
    using TFunctors::operator()...;
};
// Deduction guides are wrongly parsed by Resharper, so we need to disable wrong naming warning:
// ReSharper disable CppInconsistentNaming
template <typename... TFunctors>
VariantVisitor(TFunctors...) -> VariantVisitor<TFunctors...>;
// ReSharper restore CppInconsistentNaming
}

template <typename... TArgs>
class Variant {
    std::variant<TArgs...> mImpl;

public:
    static_assert(sizeof...(TArgs) > 0);

    constexpr Variant()                                    = default;
    constexpr ~Variant()                                   = default;
    constexpr Variant(const Variant& value)                = default;
    constexpr Variant(Variant&& value) noexcept            = default;
    constexpr Variant& operator=(const Variant& value)     = default;
    constexpr Variant& operator=(Variant&& value) noexcept = default;

    template <typename T>
    constexpr Variant(T&& value) requires(ConstructibleFrom<TArgs, T> || ...)
        : mImpl(std::forward<T>(value)) {}

    template <typename T>
    void operator=(T&& value) requires(ConstructibleFrom<TArgs, T> || ...) {
        mImpl = std::forward<T>(value);
    }

    template <typename T, typename... TConstructorArgs>
    constexpr void emplace(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        mImpl.template emplace<T>(std::forward<TConstructorArgs>(args)...);
    }

    template <typename T>
    constexpr bool holdsType() const {
        return std::holds_alternative<T>(mImpl);
    }

    constexpr int holdsIndex() const {
        return int(mImpl.index());
    }

    template <typename... TFunctors>
    decltype(auto) visit(TFunctors&&... functors) {
        return std::visit(Detail::VariantVisitor {functors...}, mImpl);
    }
    template <typename... TFunctors>
    decltype(auto) visit(TFunctors&&... functors) const {
        return std::visit(Detail::VariantVisitor {functors...}, mImpl);
    }

    bool operator==(const Variant& other) const = default;

    // =======================================================================================================
    // Getters
    // =======================================================================================================

    template <typename T>
    constexpr T* tryGet() & {
        return std::get_if<T>(&mImpl);
    }

    template <typename T>
    constexpr const T* tryGet() const& {
        return std::get_if<T>(&mImpl);
    }

    template <typename T>
    constexpr T* tryGet() && = delete;
    template <typename T>
    constexpr const T* tryGet() const&& = delete;

    template <typename T>
    constexpr T& get() & {
        BUFF_ASSERT(std::holds_alternative<T>(mImpl) == true, typeid(T).name());
        return std::get<T>(mImpl);
    }

    template <typename T>
    constexpr const T& get() const& {
        BUFF_ASSERT(std::holds_alternative<T>(mImpl) == true, typeid(T).name(), mImpl.index());
        return std::get<T>(mImpl);
    }
    template <typename T>
    constexpr T& get() && = delete;
    template <typename T>
    constexpr const T& get() const&& = delete;
};

BUFF_NAMESPACE_END
