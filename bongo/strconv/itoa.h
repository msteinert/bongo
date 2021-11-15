// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <type_traits>
#include <limits>

#include <bongo/strconv/detail/itoa.h>

namespace bongo::strconv {

template <
  typename T,
  typename OutputIt,
  typename = std::enable_if_t<std::numeric_limits<T>::is_integer && !std::is_same_v<T, bool>>
>
constexpr void format(T i, OutputIt out, long base = 10) {
  if (0 <= i && i <= static_cast<T>(detail::n_smalls) && base == 10) {
    detail::format_small(i, out);
  } else {
    detail::format_bits(i, out, base);
  }
}

}  // namespace bongo::strconv
