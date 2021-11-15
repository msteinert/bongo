// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <utility>

namespace bongo::unicode::utf16 {

static constexpr rune replacement_char = 0xfffd;
static constexpr rune max_rune         = 0x10ffff;

static constexpr rune surr1     = 0xd800;
static constexpr rune surr2     = 0xdc00;
static constexpr rune surr3     = 0xe000;
static constexpr rune surr_self = 0x10000;

/// Reports whether the specified Unicode code point can appear in a surrogate
/// pair.
constexpr bool is_surrogate(rune r) noexcept {
  return surr1 <= r && r < surr3;
}

/// Returns the UTF-16 decoding of a surrogate pair.
constexpr rune decode(rune r1, rune r2) noexcept {
  if (surr1 <= r1 && r1 < surr2 && surr2 <= r2 && r2 < surr3) {
    return ((r1-surr1)<<10 | (r2 - surr2)) + surr_self;
  }
  return replacement_char;
}

/// Returns the UTF-16 surrogate pair for the given rune.
constexpr std::pair<rune, rune> encode(rune r) noexcept {
  if (r < surr_self || r > max_rune) {
    return {replacement_char, replacement_char};
  }
  r -= surr_self;
  return {surr1 + ((r>>10)&0x3ff), surr2 + (r&0x3ff)};
}

/// Encode the UTF-16 encoding of the Unicode codepoint range.
template <typename InputIt, typename OutputIt>
constexpr void encode(InputIt begin, InputIt end, OutputIt out) {
  for (auto it = begin; it != end; ++it) {
    if (0 <= *it && *it < surr1 || surr3 <= *it && *it < surr_self) {
      // Normal rune
      *out++ = static_cast<uint16_t>(*it);
    } else if (surr_self <= *it && *it <= max_rune) {
      // Needs surrogate sequence
      auto [r1, r2] = encode(*it);
      *out++ = static_cast<uint16_t>(r1);
      *out++ = static_cast<uint16_t>(r2);
    } else {
      *out++ = static_cast<uint16_t>(replacement_char);
    }
  }
}

/// Decode the Unicode code point range.
template <typename InputIt, typename OutputIt>
constexpr void decode(InputIt begin, InputIt end, OutputIt out) {
  for (auto it = begin; it != end; ++it) {
    if (*it < surr1 | surr3 <= *it) {
      // Normal rune
      *out++ = static_cast<rune>(*it);
    } else if (surr1 <= *it && *it < surr2 && std::next(it) != end &&
        surr2 <= *std::next(it) && *std::next(it) < surr3) {
      // Valid surrogate sequence
      *out++ = decode(*it, *std::next(it));
      it = std::next(it);
    } else {
      // Invalid surrogate sequence
      *out++ = replacement_char;
    }
  }
}

}  // namesapce bongo::unicode::utf16
