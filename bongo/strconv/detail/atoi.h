// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <iterator>

namespace bongo::strconv::detail {

template <typename T>
constexpr T lower(T c) {
  return c | ('x' - 'X');
}

template <typename InputIt>
constexpr bool apostrophe_ok(InputIt begin, InputIt end) {
  if (begin == end) {
    return true;
  }
  auto saw = '^';
  if (*begin == '-' || *begin == '+') {
    begin = std::next(begin);
  }
  auto hex = false;
  if (begin != end && std::next(begin) != end) {
    auto c0 = *begin;
    if (c0 == '0') {
      auto c1 = lower(*std::next(begin));
      if (c1 == 'b' || c1 == 'o' || c1 == 'x') {
        hex = lower(*std::next(begin)) == 'x';
        begin = std::next(begin, 2);
      } else {
        saw = '0';
      }
    }
  }
  for (auto it = begin; it < end; ++it) {
    auto c = lower(*it);
    if ('0' <= c && c <= '9' || hex && 'a' <= c && c <= 'f') {
      saw = '0';
      continue;
    }
    if (c == '\'') {
      if (saw != '0') {
        return false;
      }
      saw = '\'';
      continue;
    }
    if (saw == '\'') {
      return false;
    }
    saw = '!';
  }
  return saw != '\'';
}

}  // namespace bongo::strconv::detail
