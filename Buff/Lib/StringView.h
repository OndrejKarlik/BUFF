#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Optional.h"
#include "Lib/Utils.h"
#include <string>

BUFF_NAMESPACE_BEGIN

class String;

class StringView {
    friend class String;

    std::string_view mImpl;

public:
    // =======================================================================================================
    // Constructors, destructors, conversions, comparison
    // =======================================================================================================

    constexpr StringView() = default;

    // ReSharper disable CppNonExplicitConvertingConstructor
    constexpr StringView(const char* value)
        : mImpl(value) {
        // assertValidity();
    }

    StringView(const char8_t* value)
        : mImpl(reinterpret_cast<const char*>(value)) {
        // assertValidity();
    }

    /// \param numChars Does NOT include terminating zero
    constexpr StringView(const char* value, const int numChars)
        : mImpl(value, value + numChars) {
        // assertValidity();
    }

    constexpr StringView(const std::string_view& value)
        : mImpl(value) {
        // assertValidity();
    }
    // ReSharper restore CppNonExplicitConvertingConstructor

    // TODO Emscripten does not have <=> on string_view
    constexpr bool operator<(const StringView& other) const {
        return mImpl < other.mImpl;
    }
    constexpr bool        operator==(const StringView& other) const = default;
    constexpr friend bool operator==(const StringView& x, const char* y) {
        return x.mImpl == y;
    }
    constexpr std::strong_ordering operator<=>(const StringView& other) const {
        return mImpl <=> other.mImpl;
    }

    constexpr explicit operator const std::string_view&() const {
        return mImpl;
    }

    constexpr explicit operator ArrayView<const char>() const {
        return ArrayView(mImpl.data(), mImpl.size());
    }

    /// \throws Exception if the string is not valid UTF-8
    std::wstring asWString() const;

    // =======================================================================================================
    // Basic properties
    // =======================================================================================================

    constexpr int size() const {
        // assertValidity();
        return int(mImpl.size());
    }

    constexpr bool isEmpty() const {
        return mImpl.empty();
    }
    constexpr bool notEmpty() const {
        return !isEmpty();
    }

    // =======================================================================================================
    // Element access
    // =======================================================================================================

    constexpr auto begin() const {
        return mImpl.begin();
    }
    constexpr auto end() const {
        return mImpl.end();
    }

    constexpr char operator[](const int index) const {
        BUFF_ASSERT(index >= 0 && index < size());
        return mImpl[index];
    }

    constexpr Optional<char> tryGet(const int index) const {
        if (index >= 0 && index < size()) {
            return mImpl[index];
        } else {
            return NULL_OPTIONAL;
        }
    }

    /// May NOT be null-terminated!!!
    constexpr const char* data() const {
        return mImpl.data();
    }

    // =======================================================================================================
    // Searching
    // =======================================================================================================

    constexpr bool endsWith(const StringView& pattern) const {
        return mImpl.ends_with(pattern.mImpl);
    }

    constexpr bool startsWith(const StringView& pattern) const {
        return mImpl.starts_with(pattern.mImpl);
    }

    constexpr Optional<int> findFirstOf(const StringView listOfChars) const {
        const auto result = mImpl.find_first_of(listOfChars.mImpl);
        return result != std::string::npos ? Optional(int(result)) : NULL_OPTIONAL;
    }
    constexpr Optional<int> findLastOf(const StringView listOfChars) const {
        const auto result = mImpl.find_last_of(listOfChars.mImpl);
        return result != std::string::npos ? Optional(int(result)) : NULL_OPTIONAL;
    }

    constexpr bool contains(const StringView& pattern) const {
        return mImpl.contains(pattern.mImpl);
    }

    constexpr Optional<int> find(const StringView pattern, const int startPos = 0) const {
        BUFF_ASSERT(startPos <= size());
        BUFF_ASSERT(pattern.notEmpty());
        const size_t i = mImpl.find(pattern.mImpl, startPos);
        return i != std::string_view::npos ? Optional(safeIntegerCast<int>(i)) : NULL_OPTIONAL;
    }

    constexpr Optional<int> findLast(const StringView pattern) const {
        BUFF_ASSERT(pattern.notEmpty());
        const size_t i = mImpl.rfind(pattern.mImpl);
        return i != std::string_view::npos ? Optional(safeIntegerCast<int>(i)) : NULL_OPTIONAL;
    }

    // =======================================================================================================
    // Misc
    // =======================================================================================================

    size_t getHash() const {
        return std::hash<std::string_view>()(mImpl);
    }

    [[nodiscard]] constexpr StringView getSubstring(const int           begin,
                                                    const Optional<int> count = NULL_OPTIONAL) const {
        BUFF_ASSERT(begin >= 0 && begin <= size(), begin, size());
        BUFF_ASSERT(!count || (*count >= 0 && begin + *count <= size()), begin, count);
        return mImpl.substr(begin, count ? *count : std::string::npos);
    }

    [[nodiscard]] constexpr StringView getTrimmed() const {
        const auto begin = mImpl.find_first_not_of(" \t\n\r");
        const auto end   = mImpl.find_last_not_of(" \t\n\r");
        if (begin == std::string_view::npos) {
            return {};
        }
        return getSubstring(int(begin), int(end - begin + 1));
    }

    [[nodiscard]] String getToLower() const;

    [[nodiscard]] String getToUpper() const;

    /// Will include empty strings if there are 2 delimiters in a row. This can be changed to a flag in the
    /// future.
    Array<StringView> explode(StringView delimiter) const;

private:
    constexpr void assertValidity() const {
        for (auto& i : mImpl) {
            BUFF_ASSERT(i != '\0');
        }
    }
};

inline StringView operator""_Sv(const char* utf8, const size_t size) {
    return StringView(utf8, int(size));
}

BUFF_NAMESPACE_END
