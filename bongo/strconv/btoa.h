// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <iterator>
#include <string_view>
#include <type_traits>

namespace bongo::strconv {

template <
  typename T,
  std::output_iterator<uint8_t> OutputIt,
  typename = std::enable_if_t<std::is_same_v<bool, typename std::remove_cv_t<T>>>
>
constexpr OutputIt format(T v, OutputIt out) {
  std::string_view s = v
    ? "true"
    : "false";
  return std::copy(s.begin(), s.end(), out);
}

}  // namespace bongo::strconv
