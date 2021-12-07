// Copyright The Go Authors.

#pragma once

#include <algorithm>

#include <bongo/strconv/detail/is_print.h>

namespace bongo::strconv {

constexpr bool is_print(rune r) {
  if (r <= 0xff) {
    if (0x20 <= r && r <= 0x7e) {
      return true;
    }
    if (0xa1 <= r && r <= 0xff) {
      return r != 0xad;
    }
    return false;
  }

  if (0 <= r && r < static_cast<rune>(1<<16)) {
    auto rr = static_cast<uint16_t>(r);
    auto it = std::lower_bound(detail::is_print16.begin(), detail::is_print16.end(), rr);
    auto i = std::distance(detail::is_print16.begin(), it);
    if (it == detail::is_print16.end() || rr < detail::is_print16[i&~1] || detail::is_print16[i|1] < rr) {
      return false;
    }
    return !std::binary_search(detail::is_not_print16.begin(), detail::is_not_print16.end(), rr);
  }

  auto rr = static_cast<uint32_t>(r);
  auto it = std::lower_bound(detail::is_print32.begin(), detail::is_print32.end(), rr);
  auto i = std::distance(detail::is_print32.begin(), it);
  if (it == detail::is_print32.end() || rr < detail::is_print32[i&~1] || detail::is_print32[i|1] < rr) {
    return false;
  }
  if (r >= 0x20000) {
    return true;
  }
  r -= 0x10000;
  return !std::binary_search(detail::is_not_print32.begin(), detail::is_not_print32.end(), static_cast<uint16_t>(r));
}

constexpr bool is_graphic(rune r) {
  if (is_print(r)) {
    return true;
  }
  return detail::is_in_graphic_list(r);
}

}  // namespace bongo::strconv
