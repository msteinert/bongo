// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <array>
#include <iterator>

#include <bongo/rune.h>

namespace bongo::unicode::utf8 {

static constexpr rune rune_error = 0xfffd;
static constexpr rune rune_self  = 0x80;
static constexpr rune max_rune   = 0x10ffff;
static constexpr rune utf_max    = 4;

static constexpr rune surrogate_min = 0xd800;
static constexpr rune surrogate_max = 0xdfff;

static constexpr rune t1 = 0b00000000;
static constexpr rune tx = 0b10000000;
static constexpr rune t2 = 0b11000000;
static constexpr rune t3 = 0b11100000;
static constexpr rune t4 = 0b11110000;
static constexpr rune t5 = 0b11111000;

static constexpr rune maskx = 0b00111111;
static constexpr rune mask2 = 0b00011111;
static constexpr rune mask3 = 0b00001111;
static constexpr rune mask4 = 0b00000111;

static constexpr rune rune1_max = (1<<7) - 1;
static constexpr rune rune2_max = (1<<11) - 1;
static constexpr rune rune3_max = (1<<16) - 1;

static constexpr rune locb = 0b10000000;
static constexpr rune hicb = 0b10111111;

static constexpr uint8_t xx = 0xf1;
static constexpr uint8_t as = 0xf0;
static constexpr uint8_t s1 = 0x02;
static constexpr uint8_t s2 = 0x13;
static constexpr uint8_t s3 = 0x03;
static constexpr uint8_t s4 = 0x23;
static constexpr uint8_t s5 = 0x34;
static constexpr uint8_t s6 = 0x04;
static constexpr uint8_t s7 = 0x44;

static constexpr std::array<uint8_t, 256> first = {
  //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x00-0x0F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x10-0x1F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x20-0x2F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x30-0x3F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x40-0x4F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x50-0x5F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x60-0x6F
  as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x70-0x7F
  //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
  xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x80-0x8F
  xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x90-0x9F
  xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xA0-0xAF
  xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xB0-0xBF
  xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xC0-0xCF
  s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xD0-0xDF
  s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, // 0xE0-0xEF
  s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xF0-0xFF
};

struct accept_range {
  uint8_t lo;
  uint8_t hi;
};

static constexpr std::array<accept_range, 16> accept_ranges = {
  accept_range{locb, hicb},
  accept_range{0xA0, hicb},
  accept_range{locb, 0x9F},
  accept_range{0x90, hicb},
  accept_range{locb, 0x8F},
};

/// Cast an iterator+offset to an unsigned byte.
template <typename InputIt>
constexpr uint8_t to_byte(InputIt it, size_t n = 0) noexcept {
  static_assert(sizeof *it == 1);
  return static_cast<uint8_t>(*std::next(it, n));
}

/// Cast a value to a rune.
template <typename T>
constexpr rune to_rune(T v) noexcept {
  return static_cast<rune>(v);
}

/// Report if the byte could be the first byte of an encoded rune.
template <typename T>
constexpr bool rune_start(T v) noexcept {
  static_assert(sizeof v == 1);
  return (v&0xc0) != 0x80;
}

/// Report if a range of bytes begins with a full UTF-8 encoding of a rune.
template <typename InputIt>
constexpr bool full_rune(InputIt begin, InputIt end) noexcept {
  static_assert(sizeof *begin == 1);
  auto n = std::distance(begin, end);
  if (n < 1) {
    return false;
  }
  auto x = first[to_byte(begin)];
  if (n >= static_cast<int>(x&7)) {
    return true;
  }
  auto accept = accept_ranges[x>>4];
  if (auto c = to_byte(begin, 1); n > 1 && (c < accept.lo || accept.hi < c)) {
    return true;
  } else if (auto c = to_byte(begin, 2); n > 2 && (c < locb || hicb < c)) {
    return true;
  }
  return false;
}

/// Write the UTF-8 encoding of a rune.
template <typename OutputIt>
constexpr size_t encode(rune r, OutputIt out) noexcept {
  auto i = static_cast<uint32_t>(r);
  if (i <= rune1_max) {
    *out++ = static_cast<uint8_t>(r);
    return 1;
  } else if (i <= rune2_max) {
    *out++ = t2 | static_cast<uint8_t>(r>>6);
    *out++ = tx | static_cast<uint8_t>(r)&maskx;
    return 2;
  } else if (i > max_rune || surrogate_min <= i && i <= surrogate_max) {
    *out++ = t3 | static_cast<uint8_t>(rune_error>>12);
    *out++ = tx | static_cast<uint8_t>(rune_error>>6)&maskx;
    *out++ = tx | static_cast<uint8_t>(rune_error)&maskx;
    return 3;
  } else if (i <= rune3_max) {
    *out++ = t3 | static_cast<uint8_t>(r>>12);
    *out++ = tx | static_cast<uint8_t>(r>>6)&maskx;
    *out++ = tx | static_cast<uint8_t>(r)&maskx;
    return 3;
  } else {
    *out++ = t4 | static_cast<uint8_t>(r>>18);
    *out++ = tx | static_cast<uint8_t>(r>>12)&maskx;
    *out++ = tx | static_cast<uint8_t>(r>>6)&maskx;
    *out++ = tx | static_cast<uint8_t>(r)&maskx;
    return 4;
  }
}

/// Unpack the first UTF-8 encoding in a range and return the rune and its size.
template <typename InputIt>
constexpr std::pair<rune, size_t> decode(InputIt begin, InputIt end) noexcept {
  static_assert(sizeof *begin == 1);
  auto n = std::distance(begin, end);
  if (n < 1) {
    return {rune_error, 0};
  }
  auto b0 = to_byte(begin);
  auto x = first[b0];
  if (x >= as) {
    auto mask = to_rune(x) << 31 >> 31;
    return {to_rune(b0)&~mask | rune_error&mask, 1};
  }
  auto sz = static_cast<int>(x&7);
  auto accept = accept_ranges[x>>4];
  if (n < sz) {
    return {rune_error, 1};
  }
  auto b1 = to_byte(begin, 1);
  if (b1 < accept.lo || accept.hi < b1) {
    return {rune_error, 1};
  }
  if (sz == 2) {
    return {to_rune(b0&mask2)<<6 | to_rune(b1&maskx), 2};
  }
  auto b2 = to_byte(begin, 2);
  if (b2 < locb || hicb < b2) {
    return {rune_error, 1};
  }
  if (sz == 3) {
    return {to_rune(b0&mask3)<<12 | to_rune(b1&maskx)<<6 | to_rune(b2&maskx), 3};
  }
  auto b3 = to_byte(begin, 3);
  if (b3 < locb || hicb < b3) {
    return {rune_error, 1};
  }
  return {to_rune(b0&mask4)<<18 | to_rune(b1&maskx)<<12 | to_rune(b2&maskx)<<6 | to_rune(b3&maskx), 4};
}

/// Unpack the last UTF-8 encoding in a range and return the rune and its size.
template <typename InputIt>
constexpr std::pair<rune, size_t> decode_last(InputIt begin, InputIt end) noexcept {
  static_assert(sizeof *begin == 1);
  if (std::distance(begin, end) < 1) {
    return {rune_error, 0};
  }
  auto it = std::prev(end);
  auto r0 = to_byte(it);
  if (r0 < rune_self) {
    return {r0, 1};
  }
  auto lim = std::prev(end, utf_max);
  if (lim < begin) {
    lim = begin;
  }
  for (it = std::prev(it); it >= lim; it = std::prev(it)) {
    if (rune_start(to_byte(it))) {
      break;
    }
  }
  if (it < begin) {
    it = begin;
  }
  auto [r1, size] = decode(it, end);
  if (std::next(it, size) != end) {
    return {rune_error, 1};
  }
  return {r1, size};
}

/// Returns the number of runes in a ranges of bytes.
template <typename InputIt>
constexpr size_t count(InputIt begin, InputIt end) noexcept {
  static_assert(sizeof *begin == 1);
  size_t n = 0;
  for (auto it = begin; it < end;) {
    ++n;
    auto c = to_byte(it);
    if (c < rune_self) {
      // ASCII fast path
      it = std::next(it);
      continue;
    }
    auto x = first[c];
    if (x == xx) {
      it = std::next(it);  // invalid.
      continue;
    }
    auto size = static_cast<int>(x & 7);
    if (std::next(it, size) > end) {
      it = std::next(it); // short or invalid.
      continue;
    }
    auto accept = accept_ranges[x>>4];
    if (auto c = to_byte(it, 1); c < accept.lo || accept.hi < c) {
      size = 1;
    } else if (size == 2) {
    } else if (auto c = to_byte(it, 2); c < locb || hicb < c) {
      size = 1;
    } else if (size == 3) {
    } else if (auto c = to_byte(it, 3); c < locb || hicb < c) {
      size = 1;
    }
    it = std::next(it, size);
  }
  return n;
}

/// Returns the number of bytes required to encode a rune.
constexpr int len(rune r) noexcept {
  if (r < 0) {
    return -1;
  } else if (r <= rune1_max) {
    return 1;
  } else if (r <= rune2_max) {
    return 2;
  } else if (surrogate_min <= r && r <= surrogate_max) {
    return -1;
  } else if (r <= rune3_max) {
    return 3;
  } else if (r <= max_rune) {
    return 4;
  }
  return -1;
}

/// Reports if a rune can be legally encoded as UTF-8.
constexpr bool valid(rune r) noexcept {
  if (0 <= r && r < surrogate_min) {
    return true;
  } else if (surrogate_max < r && r <= max_rune) {
    return true;
  }
  return false;
}

/// Reports if a range of bytes consists entirely of valid UTF-8 encoded runes.
template <typename InputIt>
constexpr bool valid(InputIt begin, InputIt end) noexcept {
  static_assert(sizeof *begin == 1);
  // Fast path. Check for and skip 8 bytes of ASCII characters per iteration.
  while (std::distance(begin, end) >= 8) {
    auto value =
        static_cast<uint64_t>(to_byte(begin, 0))
      | static_cast<uint64_t>(to_byte(begin, 1))
      | static_cast<uint64_t>(to_byte(begin, 2))
      | static_cast<uint64_t>(to_byte(begin, 3))
      | static_cast<uint64_t>(to_byte(begin, 4))
      | static_cast<uint64_t>(to_byte(begin, 5))
      | static_cast<uint64_t>(to_byte(begin, 6))
      | static_cast<uint64_t>(to_byte(begin, 7));
    if (value&0x80808080 != 0) {
      // Found a non ASCII byte (>= rune_self).
      break;
    }
    begin = std::next(begin, 8);
  }
  for (auto it = begin; it < end;) {
    auto si = to_byte(it);
    if (si < rune_self) {
      it = std::next(it);
      continue;
    }
    auto x = first[si];
    if (x == xx) {
      return false;  // Illegal starter byte.
    }
    auto size = static_cast<int>(x & 7);
    if (std::next(it, size) > end) {
      return false;  // Short or invalid.
    }
    auto accept = accept_ranges[x>>4];
    if (auto c = to_byte(it, 1); c < accept.lo || accept.hi < c) {
      return false;
    } else if (size == 2) {
    } else if (auto c = to_byte(it, 2); c < locb || hicb < c) {
      return false;
    } else if (size == 3) {
    } else if (auto c = to_byte(it, 3); c < locb || hicb < c) {
      return false;
    }
    it = std::next(it, size);
  }
  return true;
}

}  // namesapce bongo::unicode::utf8
