// Copyright The Go Authors.

#pragma once

#include <string_view>
#include <utility>

#include <bongo/unicode/utf8.h>

namespace bongo::strings::detail {

class ascii_set {
  uint32_t bits_[8] = {0, 0, 0, 0, 0, 0, 0, 0};

 public:
  constexpr auto contains(uint8_t c) const -> bool { return (bits_[c/32] & (1 << (c % 32))) != 0; }
  constexpr auto operator[](long i) -> uint32_t& { return bits_[i]; }
};

constexpr auto make_ascii_set(std::string_view chars) -> std::pair<ascii_set, bool> {
  namespace utf8 = unicode::utf8;
  auto as = ascii_set{};
  for (uint8_t c : chars) {
    if (c >= utf8::rune_self) {
      return {as, false};
    }
    as[c/32] |= 1 << (c % 32);
  }
  return {as, true};
}

}  // namespace bongo::strings::detail
