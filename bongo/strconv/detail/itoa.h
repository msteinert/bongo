// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

#include <bongo/math/bits.h>
#include <bongo/runtime/detail/system.h>

namespace bongo::strconv::detail {

static constexpr long n_smalls = 100;
static constexpr std::string_view small_string =
  "00010203040506070809"
  "10111213141516171819"
  "20212223242526272829"
  "30313233343536373839"
  "40414243444546474849"
  "50515253545556575859"
  "60616263646566676869"
  "70717273747576777879"
  "80818283848586878889"
  "90919293949596979899";

static constexpr std::string_view digits = "0123456789abcdefghijklmnopqrstuvwxyz";

constexpr bool is_power_of_two(long i) {
  return (i&(i-1)) == 0;
}

/// Fast path for small base-10 integers.
template <typename T, std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt format_small(T i, OutputIt out) noexcept {
  std::string_view::iterator begin, end;
  if (i < 10) {
    begin = std::next(std::begin(digits), i);
    end = std::next(std::begin(digits), i+1);
  } else {
    begin = std::next(std::begin(small_string), i*2);
    end = std::next(std::begin(small_string), i*2+2);
  }
  return std::copy(begin, end, out);
}

/// Compute the string representation of an integer in the given base.
template <typename T, std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt format_bits(T v, OutputIt out, long base) {
  if (base < 2 || base > static_cast<long>(digits.size())) {
    throw std::logic_error{"strconv: illegal integer format base"};
  }
  char a[64 + 1];
  auto i = sizeof a;
  auto neg = std::is_signed_v<T> && v < 0;
  uint64_t u = neg ? -static_cast<int64_t>(v) : v;
  if (base == 10) {
    if constexpr (runtime::detail::host_32bit) {
      while (u >= 1e9) {
        auto q = u / 1e9;
        auto us = static_cast<long unsigned>(u - q*1e9);
        for (long j = 4; j > 0; --j) {
          auto is = us % 100 * 2;
          us /= 100;
          i -= 2;
          a[i+1] = small_string[is+1];
          a[i+0] = small_string[is+0];
        }
        a[--i] = small_string[us*2+1];
        u = q;
      }
    }
    auto us = static_cast<long unsigned>(u);
    while (us >= 100) {
      auto is = us % 100 * 2;
      us /= 100;
      i -= 2;
      a[i+1] = small_string[is+1];
      a[i+0] = small_string[is+0];
    }
    auto is = us * 2;
    a[--i] = small_string[is+1];
    if (us >= 10) {
      a[--i] = small_string[is];
    }
  } else if (is_power_of_two(base)) {
    // Use shifts and masks
    auto shift = static_cast<long unsigned>(math::bits::trailing_zeros(static_cast<long unsigned>(base))) & 7;
    auto m = static_cast<long unsigned>(base) - 1;
    while (u >= static_cast<uint64_t>(base)) {
      a[--i] = digits[static_cast<long unsigned>(u)&m];
      u >>= shift;
    }
    a[--i] = digits[static_cast<long unsigned>(u)];
  } else {
    // General case
    auto b = static_cast<uint64_t>(base);
    while (u >= b) {
      auto q = u / b;
      a[--i] = digits[static_cast<long unsigned>(u-q*b)];
      u = q;
    }
    a[--i] = digits[static_cast<long unsigned>(u)];
  }
  if (neg) {
    a[--i] = '-';
  }
  return std::copy(std::next(std::begin(a), i), std::end(a), out);
}

}  // namespace bongo::strconv::detail
