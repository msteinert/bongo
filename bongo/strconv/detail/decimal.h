// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <array>
#include <algorithm>
#include <string>

#include <bongo/detail/system.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/detail/left_cheat.h>

namespace bongo::strconv::detail {

static constexpr long max_shift = (sizeof (long unsigned) * 8) - 4;
static constexpr auto powtab = std::array<long, 9>{1, 3, 6, 9, 13, 16, 19, 23, 26};

template <typename OutputIt>
constexpr void digit_zero(OutputIt out, long n) {
  for (auto i = 0; i < n; ++i) {
    *out++ = '0';
  }
}

struct decimal {
  std::array<char, 800> d{};  // digits, big-endian representation
  long nd = 0;                // number of digits used
  long dp = 0;                // decimal point
  bool neg = false;           // negative flag
  bool trunc = false;         // discarded nonzero digits beyond (0, nd]

  constexpr decimal() = default;
  constexpr decimal(uint64_t v) noexcept { assign(v); }

  constexpr void assign(uint64_t v) noexcept {
    char buf[24] = {0};
    long n = 0;
    while (v > 0) {
      auto v1 = v / 10;
      v -= 10 * v1;
      buf[n++] = static_cast<char>(v + '0');
      v = v1;
    }
    nd = 0;
    for (--n; n >= 0; --n) {
      d[nd] = buf[n];
      ++nd;
    }
    dp = nd;
    trim();
  }

  template <typename InputIt>
  constexpr bool read_float(InputIt begin, InputIt end) noexcept {
    auto it = begin;
    neg = false;
    trunc = false;

    // optional sign
    if (std::distance(begin, end) <= 0) {
      return false;
    }
    switch (*it) {
    case '+':
      it = std::next(it);
      break;
    case '-':
      neg = true;
      it = std::next(it);
      break;
    default:
      break;
    }

    // digits
    auto saw_dot = false;
    auto saw_digits = false;
    for (; it < end; ++it) {
      auto c = *it;
      if (c == '\'') {
        // Already checked.
        continue;
      } else if (c == '.') {
        saw_dot = true;
        dp = nd;
        continue;
      } else if ('0' <= c && c <= '9') {
        saw_digits = true;
        if (c == '0' && nd == 0) {
          --dp;
          continue;
        }
        if (nd < static_cast<long>(d.size())) {
          d[nd++] = c;
        } else if (c != '0') {
          trunc = true;
        }
        continue;
      }
      break;
    }
    if (!saw_digits) {
      return false;
    }
    if (!saw_dot) {
      dp = nd;
    }

    // Optional exponent moves decimal point.
    if (it < end && lower(*it) == 'e') {
      it = std::next(it);
      if (it >= end) {
        return false;
      }
      auto esign = 1;
      if (*it == '+') {
        it = std::next(it);
      } else if (*it == '-') {
        it = std::next(it);
        esign = -1;
      }
      if (it >= end || *it < '0' || *it > '9') {
        return false;
      }
      auto e = 0;
      for (; it < end && ('0' <= *it && *it <= '9' || *it == '\''); ++it) {
        if (*it == '\'') {
          // Already checked.
          continue;
        }
        if (e < 10000) {
          e = e*10 + static_cast<long>(*it) - '0';
        }
      }
      dp += e * esign;
    }

    return true;
  }

  template <typename OutputIt>
  constexpr void str(OutputIt out) const {
    auto n = 10 + nd;
    if (dp > 0) {
      n += dp;
    }
    if (dp < 0) {
      n += dp;
    }
    if (nd <= 0) {
      *out++ = '0';
    } else if (dp <= 0) {
      *out++ = '0';
      *out++ = '.';
      digit_zero(out, -dp);
      std::copy(d.begin(), std::next(d.begin(), nd), out);
    } else if (dp < nd) {
      std::copy(d.begin(), std::next(d.begin(), dp), out);
      *out++ = '.';
      std::copy(std::next(d.begin(), dp), std::next(d.begin(), nd), out);
    } else {
      std::copy(d.begin(), std::next(d.begin(), nd), out);
      digit_zero(out, dp-nd);
    }
  }

  std::string str() const {
    std::string out;
    str(std::back_inserter(out));
    return out;
  }

  constexpr void trim() noexcept {
    while (nd > 0 && d[nd-1] == '0') {
      --nd;
    }
    if (nd == 0) {
      dp = 0;
    }
  }

  constexpr void right_shift(long unsigned k) noexcept {
    long r = 0;
    long w = 0;
    long unsigned n = 0;
    for (; (n>>k) == 0; ++r) {
      if (r >= nd) {
        if (n == 0) {
          nd = 0;
          return;
        }
        while ((n>>k) == 0) {
          n = n * 10;
          ++r;
        }
        break;
      }
      auto c = static_cast<long unsigned>(d[r]);
      n = n*10 + c - '0';
    }
    dp -= (r - 1);
    long unsigned mask = (1lu << k) - 1;
    for (; r < nd; ++r) {
      auto c = static_cast<long unsigned>(d[r]);
      auto dig = n >> k;
      n &= mask;
      d[w++] = static_cast<char>(dig + '0');
      n = n*10 + c - '0';
    }
    while (n > 0) {
      auto dig = n >> k;
      n &= mask;
      if (w < static_cast<long>(d.size())) {
        d[w++] = static_cast<char>(dig + '0');
      } else if (dig > 0) {
        trunc = true;
      }
      n = n * 10;
    }
    nd = w;
    trim();
  }

  constexpr bool prefix_is_less_than(std::string_view s) const noexcept {
    for (long i = 0; i < static_cast<long>(s.size()); ++i) {
      if (i >= nd) {
        return true;
      }
      if (d[i] != s[i]) {
        return d[i] < s[i];
      }
    }
    return false;
  }

  constexpr void left_shift(long unsigned k) noexcept {
    auto delta = std::get<0>(left_cheat[k]);
    if (prefix_is_less_than(std::get<1>(left_cheat[k]))) {
      --delta;
    }
    long r = nd;
    long w = nd + delta;
    long n = 0;
    for (--r; r >= 0; --r) {
      n += (static_cast<long unsigned>(d[r]) - '0') << k;
      long quo = n / 10;
      long rem = n - 10*quo;
      --w;
      if (w < static_cast<long>(d.size())) {
        d[w] = static_cast<char>(rem + '0');
      } else if (rem != 0) {
        trunc = true;
      }
      n = quo;
    }
    while (n > 0) {
      long quo = n / 10;
      long rem = n - 10*quo;
      --w;
      if (w < static_cast<long>(d.size())) {
        d[w] = static_cast<char>(rem + '0');
      } else if (rem != 0) {
        trunc = true;
      }
      n = quo;
    }
    nd += delta;
    if (nd >= static_cast<long>(d.size())) {
      nd = static_cast<long>(d.size());
    }
    dp += delta;
    trim();
  }

  constexpr void shift(long k) noexcept {
    if (nd == 0) {
    } else if (k > 0) {
      while (k > max_shift) {
        left_shift(max_shift);
        k -= max_shift;
      }
      left_shift(k);
    } else if (k < 0) {
      while (k < -max_shift) {
        right_shift(max_shift);
        k += max_shift;
      }
      right_shift(-k);
    }
  }

  constexpr bool should_round_up(long n) const noexcept {
    if (n < 0 || n >= nd) {
      return false;
    }
    if (d[n] == '5' && n+1 == nd) {
      if (trunc) {
        return true;
      }
      return n > 0 && (d[n-1]-'0')%2 != 0;
    }
    return d[n] >= '5';
  }

  constexpr void round(long n) noexcept {
    if (n < 0 || n >= nd) {
      return;
    }
    if (should_round_up(n)) {
      round_up(n);
    } else {
      round_down(n);
    }
  }

  constexpr void round_down(long n) noexcept {
    if (n < 0 || n >= nd) {
      return;
    }
    nd = n;
    trim();
  }

  constexpr void round_up(long n) noexcept {
    if (n < 0 || n >= nd) {
      return;
    }
    for (auto i = n - 1; i >= 0; --i) {
      auto c = d[i];
      if (c < '9') {
        ++d[i];
        nd = i + 1;
        return;
      }
    }
    d[0] = '1';
    nd = 1;
    ++dp;
  }

  constexpr uint64_t rounded_integer() const noexcept {
    if (dp > 20) {
      return 0xffffffffffffffff;
    }
    long i = 0;
    uint64_t n = 0;
    for (i = 0; i < dp && i < nd; ++i) {
      n = n*10 + static_cast<uint64_t>(d[i]-'0');
    }
    for (; i < dp; ++i) {
      n *= 10;
    }
    if (should_round_up(dp)) {
      ++n;
    }
    return n;
  }

  template <typename T>
  constexpr std::pair<typename float_info<T>::integer_type, bool> float_bits() noexcept {
    // Zero is a special case.
    if (nd == 0) {
      return assemble_bits<T>(0, float_info<T>::bias(), false);
    }

    // Obvious overflow/underflow.
    if (dp > 310) {
      return overflow<T>();
    } else if (dp < -330) {
      // zero
      return assemble_bits<T>(0, float_info<T>::bias(), false);
    }

    // Scale by powers of two until in range [0.5, 1.0)
    long exp = 0;
    while (dp > 0) {
      auto n =  dp >= static_cast<long>(powtab.size()) ? 27 : powtab[dp];
      shift(-n);
      exp += n;
    }
    while (dp < 0 || dp == 0 && d[0] < '5') {
      long n = -dp >= static_cast<long>(powtab.size()) ? 27 : powtab[-dp];
      shift(n);
      exp -= n;
    }

    // Range is [0.5, 1) but floating point range is [1, 2).
    --exp;

    // Minimum representable exponent is bias+1.
    if (exp < float_info<T>::bias()+1) {
      auto n = float_info<T>::bias() + 1 - exp;
      shift(-n);
      exp += n;
    }
    if (exp-float_info<T>::bias() >= (1l<<float_info<T>::exponent_bits())-1) {
      return overflow<T>();
    }

    // Extract 1+mantbits bits.
    shift(static_cast<long>(1 + float_info<T>::mantissa_bits()));
    auto mant = rounded_integer();

    // Rounding might added a bit; shift down.
    if (mant == (2lu<<float_info<T>::mantissa_bits())) {
      mant >>= 1;
      ++exp;
      if (exp-float_info<T>::bias() >= ((1l<<float_info<T>::exponent_bits())-1)) {
        return overflow<T>();
      }
    }

    // Denormalized?
    if ((mant&(1lu<<float_info<T>::mantissa_bits())) == 0lu) {
      exp = float_info<T>::bias();
    }

    return assemble_bits<T>(mant, exp, false);
  }

  template <typename T>
  [[nodiscard]] constexpr std::pair<typename float_info<T>::integer_type, bool> overflow() const noexcept {
    long exp = (1l<<float_info<T>::exponent_bits()) - 1 + float_info<T>::bias();
    return assemble_bits<T>(0, exp, true);
  }

  template <typename T>
  [[nodiscard]] constexpr std::pair<typename float_info<T>::integer_type, bool>
  assemble_bits(typename float_info<T>::integer_type mant, long exp, bool overflow) const noexcept {
    auto bits = mant & ((static_cast<typename float_info<T>::integer_type>(1)<<float_info<T>::mantissa_bits()) - 1);
    bits |= static_cast<typename float_info<T>::integer_type>(
        (exp-float_info<T>::bias())&((1lu<<float_info<T>::exponent_bits())-1))
      << float_info<T>::mantissa_bits();
    if (neg) {
      bits |= 1lu << float_info<T>::mantissa_bits() << float_info<T>::exponent_bits();
    }
    return {bits, overflow};
  }
};

}  // namespace bongo::strconv::detail
