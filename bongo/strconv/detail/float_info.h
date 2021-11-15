// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <array>
#include <type_traits>

namespace bongo::strconv::detail {

template <typename T>
struct is_supported_float : std::integral_constant<
  bool,
  std::is_same_v<float, typename std::remove_cv_t<T>> ||
  std::is_same_v<double, typename std::remove_cv_t<T>>
> {};

template <typename T>
constexpr bool is_supported_float_v = is_supported_float<T>::value;

// Attempt to encapsulate the magic numbers and computations used in
// conversions.
template <typename T>
struct float_info {};

template <>
struct float_info<float> {
  // General float info.
  using integer_type = uint32_t;
  static constexpr long unsigned mantissa_bits() noexcept { return 23; }
  static constexpr long unsigned exponent_bits() noexcept { return 8; }
  static constexpr long bias() noexcept { return -127; }

  // Exact conversion.
  static constexpr long exact_integers() { return 7; }
  static constexpr long exact_pow10() { return 10; }
  static constexpr bool exponent_too_large(float f) { return f > 1e7 || f < -1e7; }
  static constexpr auto pow10 = std::array<float, 11>{
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10
  };

  // Eisel-Lemire implementation
  static constexpr int32_t negative_zero() { return 0x80000000; }
  static constexpr long wider_approx() { return 0x3FFFFFFFFF; }
  static constexpr long shift1() { return 38; }
  static constexpr long shift2() { return 24; }
  static constexpr long final_range() { return 0xff-1; }
  static constexpr long exponent_mask() { return 0x007fffff; }
};

template <>
struct float_info<double> {
  // General float info.
  using integer_type = uint64_t;
  static constexpr long unsigned mantissa_bits() noexcept { return 52; }
  static constexpr long unsigned exponent_bits() noexcept { return 11; }
  static constexpr long bias() noexcept { return -1023; }

  // Exact conversion.
  static constexpr long exact_integers() { return 15; }
  static constexpr long exact_pow10() { return 22; }
  static constexpr bool exponent_too_large(float f) { return f > 1e15 || f < -1e15; }
  static constexpr auto pow10 = std::array<double, 23>{
     1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22,
  };

  // Eisel-Lemire implementation
  static constexpr int64_t negative_zero() { return 0x8000000000000000; }
  static constexpr long wider_approx() { return 0x1ff; }
  static constexpr long shift1() { return 9; };
  static constexpr long shift2() { return 53; };
  static constexpr long final_range() { return 0x7ff-1; }
  static constexpr long exponent_mask() { return 0x000fffffffffffff; }
};

}  // namespace bongo::strconv::detail
