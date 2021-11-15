// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <utility>

#include <bongo/math/bits.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/detail/powers_of_ten.h>

namespace bongo::strconv::detail {

template <typename T>
constexpr std::pair<T, bool> eisel_lemire(uint64_t mantissa, long exp, bool neg) {
  // Exponent range.
  T f = 0;
  if (mantissa == 0) {
    if (neg) {
      f = math::bits::bit_cast<T>(float_info<T>::negative_zero());
    }
    return {f, true};
  }
  if (exp < static_cast<long>(detailed_powers_of_ten_min_exp10) ||
      static_cast<long>(detailed_powers_of_ten_max_exp10) < exp) {
    return {0, false};
  }

  // Normalization.
  auto clz = math::bits::leading_zeros(mantissa);
  mantissa <<= static_cast<long unsigned>(clz);
  auto ret_exp2 = static_cast<uint64_t>(
      ((217706*exp)>>16)+64+(-float_info<T>::bias())) - static_cast<uint64_t>(clz);

  // Multiplication.
  auto [xhi, xlo] = math::bits::mul64(
      mantissa, std::get<1>(detailed_powers_of_ten[exp-detailed_powers_of_ten_min_exp10]));

  // Wider approximation.
  if ((xhi&float_info<T>::wider_approx()) == float_info<T>::wider_approx() &&
      xlo+mantissa < mantissa) {
    auto [yhi, ylo] = math::bits::mul64(
        mantissa, std::get<0>(detailed_powers_of_ten[exp-detailed_powers_of_ten_min_exp10]));
    auto merged_hi = xhi;
    auto merged_lo = xlo+yhi;
    if (merged_lo < xlo) {
      ++merged_hi;
    }
    if ((merged_hi&float_info<T>::wider_approx()) == float_info<T>::wider_approx() &&
        merged_lo+1 == 0 &&
        ylo+mantissa < mantissa) {
      return {0, false};
    }
    xhi = merged_hi;
    xlo = merged_lo;
  }

  // Shifting to 54 bits (25 for float).
  auto msb = xhi >> 63;
  auto ret_mantissa = xhi >> (msb + float_info<T>::shift1());
  ret_exp2 -= 1l ^ msb;

  // Half-way ambiguity.
  if (xlo == 0 && (xhi&float_info<T>::wider_approx()) == 0 && (ret_mantissa&3) == 1) {
    return {0, false};
  }

  // From 54 to 53 bits (25 to 24 for float).
  ret_mantissa += ret_mantissa & 1;
  ret_mantissa >>= 1;
  if ((ret_mantissa>>float_info<T>::shift2()) > 0) {
    ret_mantissa >>= 1;
    ret_exp2 += 1;
  }
  if (ret_exp2-1 >= float_info<T>::final_range()) {
    return {0, false};
  }
  typename float_info<T>::integer_type ret_bits =
    ((ret_exp2<<float_info<T>::mantissa_bits()) | (ret_mantissa&float_info<T>::exponent_mask()));
  if (neg) {
    ret_bits |= float_info<T>::negative_zero();
  }
  return {math::bits::bit_cast<T>(ret_bits), true};
}

}  // namespace bongo::strconv::detail
