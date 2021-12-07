// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <limits>
#include <system_error>
#include <type_traits>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/strconv/error.h>
#include <bongo/strconv/parser.h>
#include <bongo/strconv/detail/atoi.h>

namespace bongo::strconv {

template <typename T, typename InputIt>
struct parser<T, InputIt, std::enable_if_t<
  std::numeric_limits<T>::is_integer &&
  !std::is_same_v<bool, std::remove_cv_t<T>> &&
  !std::is_signed_v<T>
>> {
  constexpr std::pair<T, std::error_code> operator()(InputIt begin, InputIt end, long base = 0) {
    if (begin == end) {
      return {0, error::syntax};
    }
    auto it = begin;
    auto base0 = base == 0;
    if (2 <= base && base <= 36) {
      // Valid base
    } else if (base == 0) {
      base = 10;
      if (*it == '0') {
        auto size = std::distance(it, end);
        if (size >= 3 && detail::lower(*std::next(it)) == 'b') {
          base = 2;
          it = std::next(it, 2);
        } else if (size >= 3 && detail::lower(*std::next(it)) == 'o') {
          base = 8;
          it = std::next(it, 2);
        } else if (size >= 3 && detail::lower(*std::next(it)) == 'x') {
          base = 16;
          it = std::next(it, 2);
        } else {
          base = 8;
          it = std::next(it);
        }
      }
    } else {
      return {0, error::base};
    }
    uint64_t cutoff;
    switch (base) {
    case 2:
      cutoff = std::numeric_limits<T>::max()/2 + 1;
      break;
    case 8:
      cutoff = std::numeric_limits<T>::max()/8 + 1;
      break;
    case 10:
      cutoff = std::numeric_limits<T>::max()/10 + 1;
      break;
    case 16:
      cutoff = std::numeric_limits<T>::max()/16 + 1;
      break;
    default:
      cutoff = std::numeric_limits<T>::max()/base + 1;
      break;
    }
    auto apostrophes = false;
    uint64_t n = 0;
    for (; it < end; ++it) {
      auto c = detail::lower(*it);
      uint8_t d;
      if (c == '\'' && base0) {
        apostrophes = true;
        continue;
      } else if ('0' <= c && c <= '9') {
        d = c - '0';
      } else if ('a' <= c && c <= 'z') {
        d = c - 'a' + 10;
      } else {
        return {0, error::syntax};
      }
      if (d >= static_cast<uint8_t>(base)) {
        return {0, error::syntax};
      }
      if (n >= cutoff) {
        // n*base overflows
        return {std::numeric_limits<T>::max(), error::range};
      }
      n *= static_cast<uint64_t>(base);
      auto n1 = n + d;
      if (n1 < n || n1 > std::numeric_limits<T>::max()) {
        // n+d overflows
        return {std::numeric_limits<T>::max(), error::range};
      }
      n = n1;
    }
    if (apostrophes && !detail::apostrophe_ok(begin, end)) {
      return {0, error::syntax};
    }
    return {static_cast<T>(n), nil};
  }
};

template <typename T, typename InputIt>
struct parser<T, InputIt, std::enable_if_t<
  std::numeric_limits<T>::is_integer &&
  !std::is_same_v<bool, std::remove_cv_t<T>> &&
  std::is_signed_v<T>
>> {
  constexpr std::pair<T, std::error_code> operator()(InputIt begin, InputIt end, long base = 0) {
    if (begin == end) {
      return {0, error::syntax};
    }
    auto it = begin;
    auto neg = false;
    if (*begin == '+') {
      it = std::next(it);
    } else if (*begin == '-') {
      neg = true;
      it = std::next(it);
    }
    auto [un, err] = parser<std::make_unsigned_t<T>, InputIt>{}(it, end, base);
    if (err != nil) {
      return {0, err};
    }
    if (!neg && un > std::numeric_limits<T>::max()) {
      return {std::numeric_limits<T>::max(), error::range};
    }
    if (neg && un > static_cast<std::make_unsigned_t<T>>(-std::numeric_limits<T>::min())) {
      return {std::numeric_limits<T>::min(), error::range};
    }
    auto n = static_cast<T>(un);
    if (neg) {
      n = -n;
    }
    return {n, nil};
  }
};

}  // namespace bongo::strconv
