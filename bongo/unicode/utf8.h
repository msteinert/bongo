// Copyright The Go Authors.

#pragma once

#include <stdexcept>

#include <bongo/unicode/utf8/iterator.h>
#include <bongo/unicode/utf8/range.h>
#include <bongo/unicode/utf8/utf8.h>

namespace bongo::unicode::utf8_literals {

constexpr rune operator""_rune(char const* s, size_t len) {
  auto [r, size] = utf8::decode(s, s+len);
  return r;
}

}  // namespace bongo::unicode::utf8_literals
