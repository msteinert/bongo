// Copyright The Go Authors.

#pragma once

#include <string_view>

#include <bongo/strings/detail/ascii_set.h>
#include <bongo/strings/index.h>
#include <bongo/unicode/utf8.h>

namespace bongo::strings::detail {

constexpr auto trim_left(std::string_view s, uint8_t c) -> std::string_view {
  while (s.size() > 0 && static_cast<uint8_t>(s[0]) == c) {
    s = s.substr(1);
  }
  return s;
}

constexpr auto trim_right(std::string_view s, uint8_t c) -> std::string_view {
  while (s.size() > 0 && static_cast<uint8_t>(s[s.size()-1]) == c) {
    s = s.substr(0, s.size()-1);
  }
  return s;
}

constexpr auto trim_left(std::string_view s, ascii_set const& as) -> std::string_view {
  while (s.size() > 0) {
    if (!as.contains(s[0])) {
      break;
    }
    s = s.substr(1);
  }
  return s;
}

constexpr auto trim_right(std::string_view s, ascii_set const& as) -> std::string_view {
  while (s.size() > 0) {
    if (!as.contains(s[s.size()-1])) {
      break;
    }
    s = s.substr(0, s.size()-1);
  }
  return s;
}

constexpr auto trim_left(std::string_view s, std::string_view cutset) -> std::string_view {
  namespace utf8 = unicode::utf8;
  while (s.size() > 0) {
    auto r = utf8::to_rune(s[0]);
    auto n = 1;
    if (r >= utf8::rune_self) {
      std::tie(r, n) = utf8::decode(s);
    }
    if (!contains(cutset, r)) {
      break;
    }
    s = s.substr(n);
  }
  return s;
}

constexpr auto trim_right(std::string_view s, std::string_view cutset) -> std::string_view {
  namespace utf8 = unicode::utf8;
  while (s.size() > 0) {
    auto r = utf8::to_rune(s[0]);
    auto n = 1;
    if (r >= utf8::rune_self) {
      std::tie(r, n) = utf8::decode_last(s);
    }
    if (!contains(cutset, r)) {
      break;
    }
    s = s.substr(0, s.size()-n);
  }
  return s;
}

}  // namespace bongo::strings::detail
