// Copyright The Go Authors.

#pragma once

#include <climits>
#include <limits>
#include <type_traits>
#include <utility>

namespace bongo::math::bits {

// Similar to C++20 bit_cast.
template <typename To, typename From>
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

constexpr auto trailing_zeros(unsigned v) -> int {
  if (v == 0) {
    return sizeof (unsigned) * CHAR_BIT;
  }
  return __builtin_ctz(v);
}

constexpr auto trailing_zeros(long unsigned v) -> int {
  if (v == 0) {
    return sizeof (long unsigned) * CHAR_BIT;
  }
  return __builtin_ctzl(v);
}

constexpr auto trailing_zeros(long long unsigned v) -> int {
  if (v == 0) {
    return sizeof (long long unsigned) * CHAR_BIT;
  }
  return __builtin_ctzll(v);
}

constexpr auto leading_zeros(unsigned v) -> int {
  if (v == 0) {
    return sizeof (unsigned) * CHAR_BIT;
  }
  return __builtin_clz(v);
}

constexpr auto leading_zeros(long unsigned v) -> int {
  if (v == 0) {
    return sizeof (long unsigned) * CHAR_BIT;
  }
  return __builtin_clzl(v);
}

constexpr auto leading_zeros(long long unsigned v) -> int {
  if (v == 0) {
    return sizeof (long long unsigned) * CHAR_BIT;
  }
  return __builtin_clzll(v);
}

template <
  typename T,
  typename = std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>>
>
constexpr auto len(T v) -> int {
  return std::numeric_limits<T>::digits - leading_zeros(v);
}

constexpr auto add64(uint64_t x, uint64_t y, uint64_t carry) -> std::pair<uint64_t, uint64_t> {
  auto sum = x + y + carry;
  auto carry_out = ((x & y) | ((x | y) & ~sum)) >> 63;
  return {sum, carry_out};
}

constexpr auto mul64(uint64_t x, uint64_t y) -> std::pair<uint64_t, uint64_t> {
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
