// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include <bongo/math/bits.h>
#include <bongo/strconv/detail/atof.h>
#include <bongo/strconv/detail/decimal.h>
#include <bongo/strconv/detail/eisel_lemire.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/error.h>
#include <bongo/strconv/parser.h>

namespace bongo::strconv {

template <typename T, typename InputIt>
constexpr std::tuple<T, InputIt, std::error_code> parse_float_prefix(InputIt begin, InputIt end) {
  if (auto [val, n, ok] = detail::special<T>(begin, end); ok) {
    return {val, std::next(begin, n), std::error_code{}};
  }
  auto [mantissa, exp, neg, trunc, hex, it, ok] = detail::read_float(begin, end);
  if (!ok) {
    return {0, it, error::syntax};
  }
  if (hex) {
    auto [f, err] = detail::atof_hex<T>(mantissa, exp, neg, trunc);
    return {f, it, err};
  }
  if (!trunc) {
    if (auto [f, ok] = detail::atof_exact<T>(mantissa, exp, neg); ok) {
      return {f, it, std::error_code{}};
    }
    if (auto [f, ok] = detail::eisel_lemire<T>(mantissa, exp, neg); ok) {
      if (!trunc) {
        return {f, it, std::error_code{}};
      }
      if (auto [fup, ok] = detail::eisel_lemire<T>(mantissa+1, exp, neg); ok && f == fup) {
        return {f, it, std::error_code{}};
      }
    }
  }
  // Slow fallback.
  auto d = detail::decimal{};
  if (!d.read_float(begin, end)) {
    return {0, it, error::syntax};
  }
  auto [bits, overflow] = d.float_bits<T>();
  auto f = math::bits::bit_cast<T>(bits);
  if (overflow) {
    return {f, it, error::range};
  }
  return {f, it, std::error_code{}};
}

template <typename T, typename InputIt>
struct parser<T, InputIt, std::enable_if_t<detail::is_supported_float_v<T>>> {
  constexpr std::pair<T, std::error_code> operator()(InputIt begin, InputIt end) {
    auto [f, it, err] = parse_float_prefix<T>(begin, end);
    if (it != end && (err == std::error_code{} || err != error::syntax)) {
      return {0, error::syntax};
    }
    return {f, err};
  }
};

}  // namespace bongo::strconv
