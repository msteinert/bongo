// Copyright The Go Authors.

#pragma once

#include <complex>
#include <concepts>
#include <exception>
#include <span>
#include <string_view>
#include <system_error>
#include <type_traits>

#include <bongo/fmt/formatter.h>
#include <bongo/fmt/stringer.h>

namespace bongo::fmt::detail {

template <typename T>
concept Boolean =
  std::same_as<std::remove_cvref_t<T>, bool>;

template <typename T>
concept Integer =
  std::integral<std::remove_cvref_t<T>> &&
  !std::same_as<std::remove_cvref_t<T>, bool>;

template <typename T>
concept Floating =
  std::same_as<std::remove_cvref_t<T>, float> ||
  std::same_as<std::remove_cvref_t<T>, double>;

template <typename T>
concept Complex =
  std::same_as<std::remove_cvref_t<T>, std::complex<float>> ||
  std::same_as<std::remove_cvref_t<T>, std::complex<double>>;

template <typename T>
concept String =
  std::same_as<std::remove_cvref_t<T>, std::string_view> ||
  std::constructible_from<std::string_view, T> ||
  std::convertible_to<T, std::string_view>;

template <typename T>
concept Bytes =
  std::same_as<std::remove_cvref_t<T>, std::span<uint8_t>> ||
  std::same_as<std::remove_cvref_t<T>, std::span<uint8_t const>> ||
  std::constructible_from<std::span<uint8_t>, T> ||
  std::constructible_from<std::span<uint8_t const>, T> ||
  std::convertible_to<T, std::span<uint8_t>> ||
  std::convertible_to<T, std::span<uint8_t const>>;

template <typename T>
concept Pointer =
  std::is_pointer_v<std::remove_cvref_t<T>> &&
  !String<T> &&
  !Bytes<T>;

template <typename T>
concept Iterable = requires (T c) {
  std::begin(c);
  std::end(c);
};

template <typename T>
concept Unprintable =
  !Boolean<T> &&
  !Integer<T> &&
  !Floating<T> &&
  !Complex<T> &&
  !String<T> &&
  !Bytes<T> &&
  !Pointer<T>;

template <typename T>
concept ErrorCodeEnum =
  std::is_error_code_enum_v<std::remove_cvref_t<T>>;

template <typename T>
concept ErrorConditionEnum =
  std::is_error_condition_enum_v<std::remove_cvref_t<T>>;

template <typename T>
concept Error =
  std::same_as<std::remove_cvref_t<T>, std::error_code> ||
  std::same_as<std::remove_cvref_t<T>, std::error_condition>;

template <typename T>
concept Exception =
  std::is_base_of_v<std::exception, T>;

template <typename>
struct is_tuple : std::false_type {};
template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};
template <typename... Ts>
constexpr bool is_tuple_v = is_tuple<Ts...>::value;

template <typename... Ts>
concept Tuple = is_tuple_v<std::remove_cvref_t<Ts>...>;

template <typename>
struct is_pair : std::false_type {};
template <typename... Ts>
struct is_pair<std::pair<Ts...>> : std::true_type {};
template <typename... Ts>
constexpr bool is_pair_v = is_pair<Ts...>::value;

template <typename... Ts>
concept Pair = is_pair_v<std::remove_cvref_t<Ts>...>;

}  // namespace bongo::fmt::detail
