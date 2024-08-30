#pragma once
#include "Lib/String.h"
#include <expected>

BUFF_NAMESPACE_BEGIN

namespace Detail {

struct UnexpectedInitializer {
    String error;
};

}

template <typename T>
class Expected {
    std::expected<T, String> mImpl;

public:
    Expected(const T& value) requires std::is_copy_constructible_v<T>
        : mImpl(value) {}
    Expected(T&& value)
        : mImpl(std::move(value)) {}

    Expected(Detail::UnexpectedInitializer unexpected)
        : mImpl(std::unexpect, std::move(unexpected.error)) {
        BUFF_ASSERT(!hasValue());
    }

    operator bool() const {
        return hasValue();
    }
    bool hasValue() const {
        return mImpl.has_value();
    }

    StringView getError() const {
        BUFF_ASSERT(!hasValue());
        return mImpl.error();
    }

    const T& operator*() const& {
        BUFF_ASSERT(hasValue());
        return *mImpl;
    }
    T&& operator*() && {
        BUFF_ASSERT(hasValue());
        return *std::move(mImpl);
    }
    const T* operator->() const {
        BUFF_ASSERT(hasValue());
        return &*mImpl;
    }
};

inline Detail::UnexpectedInitializer makeUnexpected(String error) {
    return {std::move(error)};
}

template <SerializableWithStdStreams T>
std::ostream& operator<<(std::ostream& os, const Expected<T>& value) {
    if (value) {
        os << "Expected(" << *value << ")";
    } else {
        os << "Expected(" << value.getError() << ")";
    }
    return os;
}

BUFF_NAMESPACE_END
