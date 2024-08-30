#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/Optional.h"
#include "Lib/StringView.h"
#include <sstream>
#include <string>

BUFF_NAMESPACE_BEGIN

template <NonConst T>
class Array;
class String;

template <typename T>
String toStr(T&& value) requires SerializableWithStdStreams<T>;

class String {
    Array<char>              mImpl;
    static inline const char ZERO = '\0';

public:
    ~String() = default;

    // =======================================================================================================
    // Constructors
    // =======================================================================================================
    constexpr String()          = default;
    String(const String& other) = default;
    String(String&& other)      = default;

    // ReSharper disable CppNonExplicitConvertingConstructor
    String(const char* utf8)
        : String(StringView(utf8)) {}
    String(const char* utf8, const int numChars)
        : String(StringView(utf8, numChars)) {}
    String(const char8_t* utf8)
        : String(reinterpret_cast<const char*>(utf8)) {}
    String(const char8_t* utf8, const int numChars)
        : String(reinterpret_cast<const char*>(utf8), numChars) {}
    String(const std::string& str)
        : String(StringView(str)) {}
    String(const std::string_view& str)
        : String(StringView(str)) {}
    String(const StringView& str) {
        *this = str;
    }
    String(const std::wstring& str);

    // ReSharper restore CppNonExplicitConvertingConstructor

    static String fromArray(Array<char> array) {
        String result;
        result.mImpl = std::move(array);
        result.afterModification();
        return result;
    }

    static String fromCp1252(const char* array, int numChars);
    static String fromCp1252(const char* array) {
        return fromCp1252(array, int(strlen(array)));
    }

    // =======================================================================================================
    // operator=
    // =======================================================================================================

    String& operator=(const String& other) = default;
    String& operator=(String&& other)      = default;

    String& operator=(StringView stringView);

    /// If we didn't use template + check for explicit type, these overloads would be used for x = {};,
    /// causing a crash because of nullptr
    template <typename T>
    String& operator=(const T* utf8)
        requires(std::is_same_v<std::decay_t<T>, char> || std::is_same_v<std::decay_t<T>, char8_t>) {
        return *this = StringView(utf8);
    }

    // =======================================================================================================
    // Comparisons
    // =======================================================================================================

    auto operator<=>(const String& other) const {
        return Array<char>::threeWayCompare(mImpl, other.mImpl);
    }

    bool operator==(const String& other) const = default;

    bool operator==(const char* y) const {
        return std::strcmp(asCString(), y) == 0;
    }
    bool operator==(const char8_t* y) const {
        return std::strcmp(asCString(), reinterpret_cast<const char*>(y)) == 0;
    }
    bool operator==(const StringView& other) const {
        return StringView(*this) == other;
    }

    std::strong_ordering operator<=>(const StringView& other) const {
        return StringView(*this) <=> other;
    }
    std::strong_ordering operator<=>(const char* other) const {
        return std::strcmp(asCString(), other) <=> 0;
    }

    // =======================================================================================================
    // Other basic functions
    // =======================================================================================================

    // ReSharper disable once CppNonExplicitConversionOperator
    operator StringView() const {
        return StringView(mImpl.data(), size());
    }

    const char* asCString() const {
        if (mImpl.notEmpty()) {
            BUFF_ASSERT(mImpl.size() >= 2);
            BUFF_ASSERT(mImpl.back() == '\0');
            return mImpl.data();
        } else {
            return &ZERO;
        }
    }

    /// \throws Exception if the string is not valid UTF-8
    std::wstring asWString() const;

    // =======================================================================================================
    // Basic properties
    // =======================================================================================================

    int size() const {
        BUFF_ASSERT(mImpl.size() != 1);
        return max(0, int(mImpl.size() - 1));
    }

    bool isEmpty() const {
        return mImpl.isEmpty();
    }
    bool notEmpty() const {
        return !isEmpty();
    }

    // =======================================================================================================
    // Element access
    // =======================================================================================================

    char operator[](const int index) const {
        BUFF_ASSERT(unsigned(index) < unsigned(size()));
        return mImpl[index];
    }
    char& operator[](const int index) {
        BUFF_ASSERT(unsigned(index) < unsigned(size()));
        return mImpl[index];
    }

    Optional<char> tryGet(const int index) const {
        if (unsigned(index) < unsigned(size())) {
            return mImpl[index];
        } else {
            return NULL_OPTIONAL;
        }
    }

    // =======================================================================================================
    // Searching
    // =======================================================================================================

    bool contains(const StringView& pattern) const {
        return StringView(*this).contains(pattern);
    }
    bool endsWith(const StringView& pattern) const {
        return StringView(*this).endsWith(pattern);
    }

    bool startsWith(const StringView& pattern) const {
        return StringView(*this).startsWith(pattern);
    }

    Optional<int> findFirstOf(const StringView listOfChars) const {
        return StringView(*this).findFirstOf(listOfChars);
    }
    Optional<int> findLastOf(const StringView listOfChars) const {
        return StringView(*this).findLastOf(listOfChars);
    }

    Optional<int> find(const StringView pattern, const int startPos = 0) const {
        return StringView(*this).find(pattern, startPos);
    }

    Optional<int> findLast(const StringView pattern) const {
        return StringView(*this).findLast(pattern);
    }

    // =======================================================================================================
    // Concatenation
    // =======================================================================================================

    String& operator+=(const StringView& other) {
        mImpl.reserve(size() + other.size());
        beforeModification();
        mImpl.pushBackRange(ArrayView(other));
        afterModification();
        return *this;
    }

    template <typename T>
    String& operator<<(T&& value) {
        constexpr bool IS_STRING      = std::is_same_v<std::decay_t<T>, String>;
        constexpr bool IS_STRING_VIEW = std::is_same_v<std::decay_t<T>, StringView>;
        if constexpr (IS_STRING || IS_STRING_VIEW) {
            *this += std::forward<T>(value);
        } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
            beforeModification();
            mImpl.pushBack(value);
            afterModification();
        } else {
            *this += toStr(std::forward<T>(value));
        }
        return *this;
    }

    // =======================================================================================================
    // Misc
    // =======================================================================================================

    size_t getHash() const {
        return std::hash<std::string_view>()(std::string_view(StringView(*this)));
    }

    void clear() {
        mImpl.clear();
    }

    /// \return Number of replacements
    int replaceAll(const StringView& pattern, const StringView& replacement) {
        Optional<int> startPos     = 0;
        int           replacements = 0;
        while ((startPos = find(pattern, *startPos))) {
            replace(*startPos, pattern.size(), replacement);
            startPos = *startPos + replacement.size();
            ++replacements;
        }
        return replacements;
    }

    [[nodiscard]] String getToLower() const;

    [[nodiscard]] String getToUpper() const;

    [[nodiscard]] String getQuoted() const;

    [[nodiscard]] String getWithReplaceAll(const StringView& pattern, const StringView& replacement) const;

    [[nodiscard]] StringView getSubstring(const int begin, const Optional<int> count = NULL_OPTIONAL) const {
        return StringView(*this).getSubstring(begin, count);
    }

    [[nodiscard]] StringView getTrimmed() const {
        return StringView(*this).getTrimmed();
    }

    bool isAscii() const;

    bool isValidUtf8() const;

    /// \param position must be valid index
    /// \param count    can be any positive number, even such that index+count is bigger than size(), in which
    ///                 case just the rest of the string is safely cut
    void erase(const int position, const int count = INT_MAX) {
        BUFF_ASSERT(position >= 0 && position < size());
        BUFF_ASSERT(count >= 0);
        const int originalSize = size();
        beforeModification();
        mImpl.eraseRange(position, min(originalSize - position, count));
        afterModification();
    }

    [[nodiscard]] String getWithErase(const int index, const int count = INT_MAX) const {
        String copy = *this;
        copy.erase(index, count);
        return copy;
    }

    void insert(const int position, const StringView what) {
        BUFF_ASSERT(position >= 0 && position <= size());
        beforeModification();
        mImpl.insertRange(position, ArrayView(what));
        afterModification();
    }

    [[nodiscard]] String getWithInsert(const int position, const StringView what) const {
        String res = *this;
        res.insert(position, what);
        return res;
    }

    /// \param position must be valid index or equal to size()
    /// \param count    can be any positive number, even such that index+count is bigger than size(), in which
    ///                 case just the rest of the string is safely cut
    void replace(const int position, const int count, const StringView what) {
        BUFF_ASSERT(position >= 0 && position <= size());
        BUFF_ASSERT(count >= 0);
        const int oldSize = size();
        beforeModification();
        mImpl.replaceRange(position, min(count, oldSize - position), ArrayView(what));
        afterModification();
    }

    [[nodiscard]] String getWithReplace(const int position, const int count, const StringView what) const {
        String res = *this;
        res.replace(position, count, what);
        return res;
    }

    void reserve(const int size) {
        BUFF_ASSERT(size >= 0);
        if (size > 0) {
            mImpl.reserve(size + 1);
        }
    }

    /// Will include empty strings if there are 2 delimiters in a row. This can be changed to a flag in the
    /// future.
    Array<StringView> explode(StringView delimiter) const {
        return StringView(*this).explode(delimiter);
    }

private:
    void beforeModification() {
        BUFF_ASSERT(mImpl.size() != 1);
        if (mImpl.notEmpty()) {
            BUFF_ASSERT(mImpl.back() == '\0');
            mImpl.eraseByIndex(mImpl.size() - 1);
            BUFF_ASSERT(mImpl.back() != '\0');
        }
    }
    void afterModification() {
        if (mImpl.notEmpty()) {
            BUFF_ASSERT(mImpl.back() != '\0');
            mImpl.pushBack('\0');
        }
        // assertValidity();
    }
    void assertValidity() const {
        if constexpr (BUFF_DEBUG) {
            BUFF_ASSERT(mImpl.size() != 1);
            for (const auto& i : iterate(mImpl)) {
                BUFF_ASSERT((i == '\0') == i.isLast());
            }
        }
    }
};

String operator+(const String& a, const String& b);

template <typename T>
String operator+(const T& value, const String& string) {
    String retVal = toStr(value);
    retVal << string;
    return retVal;
}
template <typename T>
String operator+(const T& value, const StringView& string) {
    String retVal = toStr(value);
    retVal << string;
    return retVal;
}
template <typename T>
String operator+(String string, const T& value)
    requires(!std::is_same_v<T, String> && !std::is_same_v<T, StringView>) {
    string << toStr(value);
    return string;
}
template <typename T>
String operator+(const StringView& string, const T& value)
    requires(!std::is_same_v<T, String> && !std::is_same_v<T, StringView>) {
    String retVal = string;
    retVal << toStr(value);
    return retVal;
}

inline std::ostream& operator<<(std::ostream& stream, const String& string) {
    stream << string.asCString();
    return stream;
}

template <typename T>
String toStr(T&& value) requires SerializableWithStdStreams<T> {
    std::stringstream tmp;
    if constexpr (std::is_floating_point_v<std::decay_t<T>>) {
        tmp.precision(99); // Increase precision for lossless print
    }
    tmp << std::forward<T>(value);
    return String(tmp.str());
}

template <typename T>
Optional<T> fromStr(const String& value) {
    std::stringstream tmp;
    tmp << value;
    T result;
    tmp >> result;
    if (!tmp.fail()) {
        return result;
    } else {
        return NULL_OPTIONAL;
    }
}

template <typename T>
Optional<T> parseNumber(const String& value, const int base) {
    char*       begin  = const_cast<char*>(value.asCString()); // Necessary for C API
    char*       end    = begin + value.size();
    const int64 result = std::strtoull(begin, &end, base);
    if (end != begin) {
        return T(result);
    } else {
        return NULL_OPTIONAL;
    }
}

String formatBigNumber(int64 number);

String toStrHexadecimal(int64 number);

Optional<int64> fromStrHexadecimal(StringView number);

struct FormatFloatParams {
    int  maxDecimals   = 3;
    bool forceDecimals = false;
    // bool separateThousands = false;
};
String formatFloat(double number, const FormatFloatParams& params);

inline bool isWhitespace(const char character) {
    return std::isspace(character) != 0;
}

inline bool isAscii(const int character) {
    return unsigned(character) < 128;
}

inline bool isDigit(const char character) {
    return character >= '0' && character <= '9';
}

// Equal to std::isAlpha, but faster
inline bool isLetter(char character) {
    character |= 32; // Make it lowercase
    return character >= 'a' && character <= 'z';
}

String leftPad(StringView input, int desiredLength, char paddingChar = ' ');

inline String operator""_S(const char* utf8, const size_t size) {
    return String(utf8, int(size));
}

struct LevenshteinDistance {
    enum class Type {
        INSERTION,
        DELETION,
        SUBSTITUTION,
    };
    using enum Type;

    struct Change {
        // Where to perform the change in source string (already modified with previous steps!)
        int where = -1;

        // Where to take the character for insertion/substitution in the target string. Not used for deletion
        int from = -1;

        // How many characters to insert/delete/substitute
        int count = -1;

        Type type = Type(-1);

        /// Applies this single change to string which had all previous changes already applied.
        void applyToFrom(String& fromWip, const StringView to) const {
            switch (type) {
            case INSERTION:
                fromWip.insert(where, to.getSubstring(from, count));
                break;
            case DELETION:
                fromWip.erase(where, count);
                break;
            case SUBSTITUTION:
                fromWip.replace(where, count, to.getSubstring(from, count));
                break;
            }
        }
    };
    int distance = 0;

    /// List of all changes that when applied consecutively modify "from" to become "to".
    Array<Change> changes;
};

LevenshteinDistance getLevenshteinDistance(const StringView& from, const StringView& to);

/// \param toStrFunctor
/// If specified, it is used to stringize each element of the list. Otherwise, the default toStr() is used.
template <IterableContainer T, typename TFunctor = std::nullptr_t>
String listToStr(const T& list, const StringView delimiter = " ", TFunctor&& toStrFunctor = nullptr)
    requires(std::is_same_v<TFunctor, std::nullptr_t> ||
             requires(TFunctor f) { String() << f(*list.begin()); }) {
    String result;
    for (auto& it : list) {
        if (result.notEmpty()) { // First
            result << delimiter;
        }
        if constexpr (std::is_same_v<TFunctor, std::nullptr_t>) {
            result << toStr(it);
        } else {
            result << toStrFunctor(it);
        }
    }
    return result;
}

BUFF_NAMESPACE_END
