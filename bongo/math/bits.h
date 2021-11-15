// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <climits>
#include <limits>
#include <type_traits>
#include <utility>

#include <bongo/detail/system.h>

namespace bongo::math::bits {

// Similar to C++20 bit_cast.
template <class To, class From>
std::enable_if_t<
  sizeof (To) == sizeof (From) &&
  std::is_trivially_copyable_v<From> &&
  std::is_trivially_copyable_v<To>,
To> constexpr bit_cast(const From& src) noexcept {
  struct {
    union {
      From from;
      To to;
    };
  } cast{src};
  return cast.to;
}

constexpr int trailing_zeros(uint32_t v) {
  if (v == 0) {
    return sizeof (uint32_t) * CHAR_BIT;
  }
  return __builtin_ctzl(v);
}

constexpr int trailing_zeros(uint64_t v) {
  if (v == 0) {
    return sizeof (uint64_t) * CHAR_BIT;
  }
  return __builtin_ctzl(v);
}

constexpr int leading_zeros(uint32_t v) {
  if (v == 0) {
    return sizeof (int32_t) * CHAR_BIT;
  }
  return __builtin_clz(v);
}

constexpr int leading_zeros(uint64_t v) {
  if (v == 0) {
    return sizeof (uint32_t) * CHAR_BIT;
  }
  return __builtin_clzl(v);
}

template <
  typename T,
  typename = std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>>
>
constexpr int len(T v) {
  return std::numeric_limits<T>::digits - leading_zeros(v);
}

constexpr std::pair<uint64_t, uint64_t> add64(uint64_t x, uint64_t y, uint64_t carry) {
  auto sum = x + y + carry;
  auto carry_out = ((x & y) | (x | y) & ~sum) >> 63;
  return {sum, carry_out};
}

constexpr std::pair<uint64_t, uint64_t> mul64(uint64_t x, uint64_t y) {
  auto const mask32 = (1lu<<32) - 1;
  auto x0 = x & mask32;
  auto x1 = x >> 32;
  auto y0 = y & mask32;
  auto y1 = y >> 32;
  auto w0 = x0 * y0;
  auto t = x1*y0 + (w0>>32);
  auto w1 = t & mask32;
  auto w2 = t >> 32;
  w1 += x0 * y1;
  auto hi = x1*y1 + w2 + (w1>>32);
  auto lo = x * y;
  return {hi, lo};
}

}  // namespace bongo::math::bits
