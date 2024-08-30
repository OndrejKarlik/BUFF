#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/TypeTraits.h"

BUFF_NAMESPACE_BEGIN

template <typename T>
concept NonConst = !std::is_const_v<std::remove_reference_t<T>>;

template <typename T>
concept Integer = std::is_integral_v<T>;

template <typename T>
concept UnsignedInteger = std::is_integral_v<T> && std::is_unsigned_v<T>;

// Concept for a type with operator< defined. Note that STL only has one for <=>
template <typename T, typename T2 = T>
concept LessThanComparable = requires(const T& a, const T2& b) {
    { a < b } -> std::convertible_to<bool>;
};

// Note that this is different from std::equality_comparable_with, which requires a common reference type
template <typename T, typename T2 = T>
concept EqualsComparable = requires(const T& a, const T2& b) {
    { a == b } -> std::convertible_to<bool>;
};

template <typename T>
concept Pointer = std::is_pointer_v<T>;

template <typename T>
concept IterableContainer = requires(const T& a) {
    { std::begin(a) };
    { std::end(a) };
    { *std::begin(a) };
    { *std::end(a) };
};

template <typename T>
concept SerializableWithStdStreams = requires(std::stringstream& ss, const T& t) {
    { ss << t };
};

/// We need this because std:::constructible_from is bugged in clang: it reports undefined types which are
/// clearly defined
template <typename T, typename... TConstructorArgs>
concept ConstructibleFrom = requires(TConstructorArgs&&... args) {
    { T(std::forward<TConstructorArgs>(args)...) };
};

template <typename T>
concept ClassEnum = IsClassEnum<T>;

BUFF_NAMESPACE_END
