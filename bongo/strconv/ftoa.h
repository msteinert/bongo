// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <type_traits>

#include <bongo/math.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/detail/ftoa.h>
#include <bongo/strconv/detail/ftoa_ryu.h>

namespace bongo::strconv {

template <
  typename T,
  typename OutputIt,
  typename = std::enable_if_t<detail::is_supported_float_v<T>>
>
constexpr void format(T f, OutputIt out, char fmt = 'e', long prec = -1) {
  auto bits = math::float_bits(f);
  auto neg = bits>>(detail::float_info<T>::exponent_bits()+detail::float_info<T>::mantissa_bits()) != 0;
  auto exp = static_cast<long>(bits>>detail::float_info<T>::mantissa_bits()) & ((1lu<<detail::float_info<T>::exponent_bits()) - 1);
  auto mant = bits & (1llu<<detail::float_info<T>::mantissa_bits()) - 1;
  if (exp == (1lu<<detail::float_info<T>::exponent_bits()) - 1) {
    std::string_view s;
    if (mant != 0) {
      s = "NaN";
    } else if (neg) {
      s = "-Inf";
    } else {
      s = "+Inf";
    }
    std::copy(s.begin(), s.end(), out);
    return;
  } else if (exp == 0) {
    ++exp;
  } else {
    mant |= static_cast<uint64_t>(1) << detail::float_info<T>::mantissa_bits();
  }
  exp += detail::float_info<T>::bias();
  if (fmt == 'b') {
    detail::fmt_b<T>(out, neg, mant, exp);
    return;
  }
  if (fmt == 'x' || fmt == 'X') {
    detail::fmt_x<T>(out, prec, fmt, neg, mant, exp);
    return;
  }
  if (prec < 0) {
    // Use Ryu algorithm.
    auto d = detail::ryu_ftoa_shortest<T>(mant, exp-static_cast<long>(detail::float_info<T>::mantissa_bits()));
    switch (fmt) {
    case 'e':
    case 'E':
      prec = std::max(d.nd-1, 0l);
      break;
    case 'f':
      prec = std::max(d.nd-d.dp, 0l);
      break;
    case 'g':
    case 'G':
      prec = d.nd;
      break;
    }
    detail::format_decimal(out, true, neg, d, prec, fmt);
    return;
  } else if (fmt != 'f'){
    // Fixed number of digits.
    auto digits = prec;
    switch (fmt) {
    case 'e':
    case 'E':
      ++digits;
      break;
    case 'g':
    case 'G':
      if (prec == 0) {
        prec = 1;
      }
      digits = prec;
      break;
    }
    if constexpr (std::is_same_v<float, std::remove_cv_t<T>>) {
      if (digits <= 9) {
        auto d = detail::ryu_ftoa_fixed32(
            static_cast<uint32_t>(mant),
            exp-static_cast<long>(detail::float_info<T>::mantissa_bits()),
            digits);
        detail::format_decimal(out, false, neg, d, prec, fmt);
        return;
      }
    }
    if constexpr (std::is_same_v<double, std::remove_cv_t<T>>) {
      if (digits <= 18) {
        auto d = detail::ryu_ftoa_fixed64(
            mant, exp-static_cast<long>(detail::float_info<T>::mantissa_bits()),
            digits);
        detail::format_decimal(out, false, neg, d, prec, fmt);
        return;
      }
    }
  }
  detail::big_ftoa<T>(out, prec, fmt, neg, mant, exp);
}

}  // namespace bongo::strconv
