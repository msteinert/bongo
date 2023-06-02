// Copyright The Go Authors.

#pragma once

#include <string_view>

#include <bongo/strings/concepts.h>
#include <bongo/unicode/utf8.h>

namespace bongo::strings::detail {

template <typename T> requires IndexFunction<T>
constexpr auto index(std::string_view s, T&& func, bool truth) -> std::string_view::size_type {
  namespace utf8 = unicode::utf8;
  for (auto [i, r] : utf8::range{s}) {
    if (func(r) == truth) {
      return i;
    }
  }
  return std::string_view::npos;
}

template <typename T> requires IndexFunction<T>
constexpr auto last_index(std::string_view s, T&& func, bool truth) -> std::string_view::size_type {
  namespace utf8 = unicode::utf8;
  for (std::string_view::size_type i = s.size(); i > 0;) {
    auto [r, size] = utf8::decode(s.substr(0, i));
    i -= size;
    if (func(r) == truth) {
      return i;
    }
  }
  return std::string_view::npos;
}

}  // namespace bongo::strings::detail
