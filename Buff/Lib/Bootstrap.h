#pragma once
#include <algorithm>
#include <cstdint>
#include <sstream>

#define BUFF_NAMESPACE_BEGIN namespace Buff {
#define BUFF_NAMESPACE_END   }

BUFF_NAMESPACE_BEGIN

class String;

constexpr const char* BUFF_LIBRARY_NAME = "BUFF";

#ifdef __clang__
#    define BUFF_PRAGMA(x) _Pragma(#x)
#    define BUFF_DISABLE_CLANG_WARNING_BEGIN(warningString)                                                  \
        BUFF_PRAGMA(clang diagnostic push)                                                                   \
        BUFF_PRAGMA(clang diagnostic ignored warningString)
#    define BUFF_DISABLE_CLANG_WARNING_END() BUFF_PRAGMA(clang diagnostic pop)
#    define BUFF_DISABLE_MSVC_WARNING_BEGIN(...)
#    define BUFF_DISABLE_MSVC_WARNING_END()
#else
#    define BUFF_DISABLE_MSVC_WARNING_BEGIN(...)                                                             \
        __pragma(warning(push)) __pragma(warning(disable : __VA_ARGS__))
#    define BUFF_DISABLE_MSVC_WARNING_END() __pragma(warning(pop))
#    define BUFF_DISABLE_CLANG_WARNING_BEGIN(warningString)
#    define BUFF_DISABLE_CLANG_WARNING_END()
#endif

static_assert(BUFF_DEBUG + BUFF_RELEASE == 1, "Exactly one must be defined");

// ReSharper disable CppInconsistentNaming
using int8   = std::int8_t;
using uint8  = std::uint8_t;
using int16  = std::int16_t;
using uint16 = std::uint16_t;
using uint   = std::uint32_t;
using int64  = std::int64_t;
using uint64 = std::uint64_t;
// ReSharper restore CppInconsistentNaming

#ifdef NAN
#    undef NAN
#endif
inline constexpr float  NAN        = std::numeric_limits<float>::signaling_NaN();
inline constexpr double DOUBLE_NAN = std::numeric_limits<double>::signaling_NaN();
#ifdef FLT_EPSILON
#    undef FLT_EPSILON
#endif
inline constexpr float FLT_EPSILON =
    1.192092896e-07f; // from <float.h>: smallest such that 1.0+FLT_EPSILON != 1.0

template <int TLength>
struct StringLiteral {
    consteval StringLiteral(const char (&str)[TLength]) {
        std::copy_n(str, TLength, value);
    }
    char value[TLength];

    consteval operator const char*() const {
        return value;
    }
};

#define BUFF_STRINGIFY(...)         BUFF_STRINGIFY_IMPL(__VA_ARGS__)
#define BUFF_STRINGIFY_IMPL(...)    #__VA_ARGS__
#define BUFF_CONCATENATE_IMPL(x, y) x##y
#define BUFF_CONCATENATE(x, y)      BUFF_CONCATENATE_IMPL(x, y)

// ===========================================================================================================
// Asserts
// ===========================================================================================================

#if BUFF_DEBUG

namespace Detail {

void handleAssert(const char* condition, const char* filename, int lineNumber, const std::stringstream& args);

template <typename... T>
std::stringstream stringify(const char* params, T&&... args)
    requires requires(std::stringstream& ss, T&&... t) {
        { ((ss << t), ...) };
    } {
    std::stringstream stream;
    stream << "Params: " << params << "\nValues: ";
    int  i     = 0;
    auto apply = [&](auto&& value) {
        stream << value;
        if (++i != sizeof...(args)) {
            stream << " | ";
        }
    };
    (apply(std::forward<T>(args)), ...);
    return stream;
}

template <size_t TExpected, size_t TActual>
constexpr void checkSizeofExpectedActual() {
    static_assert(TExpected == TActual,
                  "Wrong sizeof(this) - code around this statement needs a specific size of object, "
                  "possibly because it is manually iterating over all its members or casting it to a "
                  "memory blob.");
}

template <StringLiteral T>
void assertPrintDummy() {}

} // namespace Detail

#    define BUFF_ASSERT(condition, ...)                                                                      \
        if (!(condition)) {                                                                                  \
            /* if constexpr (std::is_constant_evaluated()) { // TODO: Do some printout of the error          \
                // static_assert(TEMPLATED_FALSE<decltype(condition)>,                                       \
                              "ASSERT failed in constexpr context: " #condition);                            \
                throw("ASSERT failed in constexpr context: " #condition);                                    \
            } else { */                                                                                      \
            ::Buff::Detail::assertPrintDummy<::Buff::StringLiteral(#condition                                \
                                                                   " " BUFF_STRINGIFY(__VA_ARGS__))>();      \
            [[unlikely]] ::Buff::Detail::handleAssert(                                                       \
                #condition,                                                                                  \
                __FILE__,                                                                                    \
                __LINE__,                                                                                    \
                ::Buff::Detail::stringify(BUFF_STRINGIFY(__VA_ARGS__) __VA_OPT__(, ) __VA_ARGS__));          \
            /*}*/                                                                                            \
        }                                                                                                    \
        static_assert(true) /* force semicolon */

#    define BUFF_ASSERT_SIZEOF(expected)                                                                     \
        Detail::checkSizeofExpectedActual<(expected), sizeof(*this)>() /* force semicolon  */

#elif BUFF_RELEASE
#    define BUFF_ASSERT(condition, ...)                                                                      \
        BUFF_DISABLE_MSVC_WARNING_BEGIN(4834)                                                                \
        (void)sizeof(condition __VA_OPT__(, ) __VA_ARGS__);                                                  \
        BUFF_DISABLE_MSVC_WARNING_END()                                                                      \
        static_assert(true) /* force semicolon */

#    define BUFF_ASSERT_SIZEOF(expected)
#else
#    error Build system broken
#endif

struct AssertArguments {
    const char* condition;
    const char* filename;
    int         lineNumber;
    const char* arguments;

    String getMessage() const;
};

#define BUFF_CHECKED_CALL(expected, ...)                                                                     \
    {                                                                                                        \
        [[maybe_unused]] const auto expected2_ = (expected);                                                 \
        [[maybe_unused]] const auto result_    = __VA_ARGS__;                                                \
        BUFF_ASSERT(expected2_ == result_, #expected, #__VA_ARGS__, expected2_, result_);                    \
    }                                                                                                        \
    static_assert(true) /* force semicolon */

#define BUFF_STOP throw ::Buff::StopException(__FUNCTION__, __FILE__, __LINE__)

// ===========================================================================================================
// Rest
// ===========================================================================================================

#define BUFF_UNUSED(...)

class Noncopyable {
public:
    Noncopyable() = default;

    Noncopyable(const Noncopyable& other)            = delete;
    Noncopyable& operator=(const Noncopyable& other) = delete;
    Noncopyable(Noncopyable&& other)                 = delete;
    Noncopyable& operator=(Noncopyable&& other)      = delete;
};

class NoncopyableMovable {
public:
    NoncopyableMovable() = default;

    NoncopyableMovable(const NoncopyableMovable& other)            = delete;
    NoncopyableMovable& operator=(const NoncopyableMovable& other) = delete;

    NoncopyableMovable(NoncopyableMovable&& other)                         = default;
    NoncopyableMovable& operator=(NoncopyableMovable&& BUFF_UNUSED(other)) = default;
};

/// Adds a virtual destructor to the class, forcing creation of the vptr table
class Polymorphic {
public:
    Polymorphic()                                    = default;
    Polymorphic(const Polymorphic& other)            = default;
    Polymorphic& operator=(const Polymorphic& other) = default;
    Polymorphic(Polymorphic&& other)                 = default;
    Polymorphic& operator=(Polymorphic&& other)      = default;

    virtual ~Polymorphic() = default;
};

using std::max;
using std::min;

/// A dummy to be used in static_asserts inside constexpr ifs. Using just false (independent of any template
/// type) would be a compile error inside an unreachable branch.
template <typename T>
constexpr bool TEMPLATED_FALSE = false;

BUFF_NAMESPACE_END
