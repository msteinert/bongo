// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <limits>
#include <string_view>
#include <tuple>

#include <bongo/math/bits.h>
#include <bongo/strconv/detail/atoi.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/error.h>

namespace bongo::strconv::detail {

template <std::input_iterator InputIt>
long common_prefix_ignore_case(InputIt begin, InputIt end, std::string_view prefix) {
  auto n = std::distance(begin, end);
  if (n > static_cast<ssize_t>(prefix.size())) {
    n = prefix.size();
  }
  for (long i = 0; i < n; ++i) {
    auto c = *begin;
    begin = std::next(begin);
    if ('A' <= c && c <= 'Z') {
      c += 'a' - 'A';
    }
    if (c != prefix[i]) {
      return i;
    }
  }
  return n;
}

template <typename T, std::input_iterator InputIt>
std::tuple<double, long, bool> special(InputIt begin, InputIt end) {
  if (std::distance(begin, end) == 0) {
    return {0, 0, false};
  }
  long n = 0;
  auto neg = false;
  auto nsign = 0;
  switch (*begin) {
  case '+':
  case '-':
    if (*begin == '-') {
      neg = true;
    }
    nsign = 1;
    begin = std::next(begin);
    [[fallthrough]];  /* fallthrough */
  case 'i':
  case 'I':
      n = common_prefix_ignore_case(begin, end, "infinity");
      if (3 < n && n < 8) {
        n = 3;
      }
      if (n == 3 || n == 8) {
        return {
          neg
            ? -std::numeric_limits<T>::infinity()
            : std::numeric_limits<T>::infinity(),
          nsign + n, true
        };
      }
    break;
  case 'n':
  case 'N':
    n = common_prefix_ignore_case(begin, end, "nan");
    if (n == 3) {
      return {std::numeric_limits<T>::quiet_NaN(), 3, true};
    }
    break;
  default:
    break;
  }
  return {0, 0, false};
}

template <std::input_iterator InputIt>
std::tuple<uint64_t, long, bool, bool, bool, InputIt, bool> read_float(InputIt begin, InputIt end) {
  auto it = begin;
  uint64_t mantissa = 0;
  long exp = 0;
  auto neg = false;
  auto trunc = false;
  auto hex = false;
  auto len = std::distance(begin, end);
  auto apostrophes = false;

  if (len == 0) {
    return {mantissa, exp, neg, trunc, hex, it, false};
  }

  // optional sign
  switch (*it) {
  case '+':
    it = std::next(it);
    break;
  case '-':
    it = std::next(it);
    neg = true;
    break;
  default:
    break;
  }

  // digits
  uint64_t base = 10;
  auto max_mant_digits = 19;
  auto exp_char = 'e';
  if (len >= 2 && *it == '0' && lower(*std::next(it)) == 'x') {
    base = 16;
    max_mant_digits = 16;
    exp_char = 'p';
    hex = true;
    it = std::next(it, 2);
  }

  auto saw_dot = false;
  auto saw_digits = false;
  auto nd = 0;
  auto nd_mant = 0;
  auto dp = 0;

  for (; it < end; it = std::next(it)) {
    auto c = *it;
    if (c == '\'') {
      apostrophes = true;
      continue;
    } else if (c == '.') {
      if (saw_dot) {
        break;
      }
      saw_dot = true;
      dp = nd;
      continue;
    } else if ('0' <= c && c <= '9') {
      saw_digits = true;
      if (c == '0' && nd == 0) {
        --dp;
        continue;
      }
      ++nd;
      if (nd_mant < max_mant_digits) {
        mantissa *= base;
        mantissa += static_cast<uint64_t>(c - '0');
        ++nd_mant;
      } else if (c != '0') {
        trunc = true;
      }
      continue;
    } else if (base == 16 && 'a' <= lower(c) && lower(c) <= 'f') {
      saw_digits = true;
      ++nd;
      if (nd_mant < max_mant_digits) {
        mantissa *= 16;
        mantissa += static_cast<uint64_t>(lower(c) - 'a' + 10);
        ++nd_mant;
      } else {
        trunc = true;
      }
      continue;
    }
    break;
  }
  if (!saw_digits) {
    return {mantissa, exp, neg, trunc, hex, it, false};
  }
  if (!saw_dot) {
    dp = nd;
  }
  if (base == 16) {
    dp *= 4;
    nd_mant *= 4;
  }

  // Optional exponent moves decimal point.
  if (it < end && lower(*it) == exp_char) {
    it = std::next(it);
    if (it >= end) {
      return {mantissa, exp, neg, trunc, hex, it, false};
    }
    auto esign = 1;
    if (*it == '+') {
      it = std::next(it);
    } else if (*it == '-') {
      it = std::next(it);
      esign = -1;
    }
    if (it >= end || *it < '0' || *it > '9') {
      return {mantissa, exp, neg, trunc, hex, it, false};
    }
    auto e = 0;
    for (; it < end && (('0' <= *it && *it <= '9') || *it == '\''); it = std::next(it)) {
      if (*it == '\'') {
        apostrophes = true;
        continue;
      }
      if (e < 10000) {
        e = e*10 + static_cast<long>(*it) - '0';
      }
    }
    dp += e * esign;
  } else if (base == 16) {
    // Must have exponent.
    return {mantissa, exp, neg, trunc, hex, it, false};
  }

  if (mantissa != 0) {
    exp = dp - nd_mant;
  }

  if (apostrophes && !apostrophe_ok(begin, it)) {
    return {mantissa, exp, neg, trunc, hex, it, false};
  }

  return {mantissa, exp, neg, trunc, hex, it, true};
}

template <typename T>
std::pair<T, bool> atof_exact(uint64_t mantissa, long exp, bool neg) {
  if (mantissa>>float_info<T>::mantissa_bits() != 0) {
    return {0, false};
  }
  auto f = static_cast<T>(mantissa);
  if (neg) {
    f = -f;
  }
  if (exp == 0) {
    return {f, true};
  } else if (exp > 0 && exp <= float_info<T>::exact_integers()+float_info<T>::exact_pow10()) {
    if (exp > float_info<T>::exact_pow10()) {
      f *= float_info<T>::pow10[exp-float_info<T>::exact_pow10()];
      exp = float_info<T>::exact_pow10();
    }
    if (float_info<T>::exponent_too_large(f)) {
      return {0, false};
    }
    return {f * float_info<T>::pow10[exp], true};
  } else if (exp < 0 && exp > -float_info<T>::exact_pow10()) {
    return {f / float_info<T>::pow10[-exp], true};
  }
  return {0, false};
}

template <typename T>
std::pair<T, std::error_code> atof_hex(uint64_t mantissa, long exp, bool neg, bool trunc) {
  auto max_exp = (1<<float_info<T>::exponent_bits()) + float_info<T>::bias() - 2;
  auto min_exp = float_info<T>::bias() + 1;
  exp += static_cast<long>(float_info<T>::mantissa_bits());

  // Shift mantissa and exponent to bring representation into float range.
  while (mantissa != 0 && (mantissa>>(float_info<T>::mantissa_bits()+2)) == 0) {
    mantissa <<= 1;
    --exp;
  }
  if (trunc) {
    mantissa |= 1;
  }
  while ((mantissa>>(1l+float_info<T>::mantissa_bits()+2)) != 0) {
    mantissa = (mantissa>>1) | (mantissa&1);
    ++exp;
  }

  while ((mantissa > 1) && exp < min_exp-2) {
    mantissa = (mantissa>>1) | (mantissa&1);
    ++exp;
  }

  auto round = mantissa & 3;
  mantissa >>= 2;
  round |= mantissa & 1;
  exp += 2;
  if (round == 3) {
    ++mantissa;
    if (mantissa == (1l<<(1+float_info<T>::mantissa_bits()))) {
      mantissa >>= 1;
      ++exp;
    }
  }

  if ((mantissa>>float_info<T>::mantissa_bits()) == 0) {
    exp = float_info<T>::bias();
  }
  std::error_code err;
  if (exp > max_exp) {
    mantissa = (1l << float_info<T>::mantissa_bits());
    exp = max_exp + 1;
    err = error::range;
  }

  auto bits = static_cast<typename float_info<T>::integer_type>(
      mantissa & ((1l<<float_info<T>::mantissa_bits()) - 1));
  bits |= static_cast<typename float_info<T>::integer_type>(
      (exp-float_info<T>::bias())&((1l<<float_info<T>::exponent_bits())-1))
    << float_info<T>::mantissa_bits();
  if (neg) {
    bits |= (1l << float_info<T>::mantissa_bits()) << float_info<T>::exponent_bits();
  }
  return {math::bits::bit_cast<T>(bits), err};
}

}  // namespace bongo::strconv::detail
