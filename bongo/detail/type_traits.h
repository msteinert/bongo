// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <istream>
#include <type_traits>

namespace bongo::detail {

template <typename T, typename = void>
struct can_istream : std::false_type {};

template <typename T>
struct can_istream<
  T,
  decltype(static_cast<std::istream&>(std::declval<std::istream&>() << std::declval<T>()))
> : std::true_type {};

template <typename T>
inline constexpr bool can_istream_v = can_istream<T>::value;

}  // namespace bongo::detail
