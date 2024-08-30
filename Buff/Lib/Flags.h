#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/ArrayView.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
class Flags {
    static_assert(std::is_enum_v<T>);

    using Underlying = std::underlying_type_t<T>;
    Underlying mValue;

public:
    constexpr Flags()
        : mValue(0) {}

    // ReSharper disable once CppNonExplicitConvertingConstructor
    template <typename... TArgs>
    constexpr Flags(TArgs... flagsToSet)
        : mValue((... | Underlying(flagsToSet))) {
        static_assert((... && std::is_same_v<T, TArgs>));
    }

    constexpr bool hasFlag(const T flag) const {
        return (mValue & Underlying(flag)) != 0;
    }

    constexpr bool hasAnyFlag(const ArrayView<const T> flags) const {
        for (auto& i : flags) {
            if (hasFlag(i)) {
                return true;
            }
        }
        return false;
    }

    constexpr auto toUnderlying() const {
        return mValue;
    }

    constexpr Flags operator|(const T& flag) const {
        Flags result = *this;
        result.mValue |= Underlying(flag);
        return result;
    }
    constexpr void clearFlag(const T& flag) {
        mValue &= ~Underlying(flag);
    }
    friend constexpr Flags operator|(const T& flag, const Flags& flags) {
        Flags result = flags;
        result.mValue |= Underlying(flag);
        return result;
    }
    constexpr Flags& operator|=(const T& flag) {
        mValue |= Underlying(flag);
        return *this;
    }
};

template <typename T, typename... TRest>
// ReSharper disable once CppInconsistentNaming - bug in Resharper, things Flags is a parameter
Flags(T, TRest...) -> Flags<T>;

BUFF_NAMESPACE_END
