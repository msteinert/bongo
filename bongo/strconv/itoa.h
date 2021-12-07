// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <limits>
#include <type_traits>

#include <bongo/strconv/detail/itoa.h>

namespace bongo::strconv {

template <
  typename T,
  std::output_iterator<uint8_t> OutputIt,
  typename = std::enable_if_t<std::numeric_limits<T>::is_integer && !std::is_same_v<T, bool>>
>
constexpr OutputIt format(T i, OutputIt out, long base = 10) {
  if (0 <= i && i <= static_cast<T>(detail::n_smalls) && base == 10) {
    return detail::format_small(i, out);
  } else {
    return detail::format_bits(i, out, base);
  }
}

}  // namespace bongo::strconv
