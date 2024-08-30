#pragma once
#include "Lib/Bootstrap.h"
#include <type_traits>

BUFF_NAMESPACE_BEGIN

namespace Detail {
template <typename T1, typename T2, typename = void>
struct IsEqualsComparable : std::false_type {};

template <typename T1, typename T2>
struct IsEqualsComparable<
    T1,
    T2,
    std::enable_if_t<true, std::void_t<decltype(std::declval<const T1&>() == std::declval<const T2&>())>>>
    : std::true_type {};

template <typename T1, typename T2, typename = void>
struct IsThreeWayComparable : std::false_type {};
template <typename T1, typename T2>
struct IsThreeWayComparable<
    T1,
    T2,
    std::enable_if_t<true, decltype(std::declval<const T1&>() <=> std::declval<const T2&>(), (void)0)>>
    : std::true_type {};

} // namespace Detail

// ReSharper disable CppInconsistentNaming
template <typename T1, typename T2 = T1>
constexpr bool IsEqualsComparable = Detail::IsEqualsComparable<T1, T2>::value;

template <typename T1, typename T2 = T1>
constexpr bool IsThreeWayComparable = Detail::IsThreeWayComparable<T1, T2>::value;

template <typename T>
constexpr bool IsClassEnum = std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

using TrueType = std::true_type;

// ReSharper restore CppInconsistentNaming

BUFF_NAMESPACE_END
