// Copyright The Go Authors.

#pragma once

#include <complex>
#include <iterator>
#include <system_error>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/strconv/atof.h>
#include <bongo/strconv/error.h>
#include <bongo/strconv/parser.h>

namespace bongo::strconv {

template <typename T, std::input_iterator InputIt>
struct parser<std::complex<T>, InputIt> {
  constexpr std::pair<std::complex<T>, std::error_code> operator()(InputIt begin, InputIt end) {
    // Remove parentheses, if any.
    if (std::distance(begin, end) >= 2 && *begin == '(' && *std::prev(end) == ')') {
      begin = std::next(begin);
      end = std::prev(end);
    }

    std::error_code pending = nil;  // possibly pending range error

    // Read real part (possibly imaginery part if followed by 'i').
    auto [re, it, err] = parse_float_prefix<T>(begin, end);
    if (err != nil) {
      if (err != error::range) {
        return {0, err};
      }
      pending = error::range;
    }

    // If we have nothing left, we're done.
    if (std::distance(it, end) == 0) {
      return {{re, 0}, pending};
    }

    // Otherwise, look at the next character.
    switch (*it) {
    case '+':
      if (std::distance(it, end) > 1 && *std::next(it) != '+') {
        it = std::next(it);
      }
      break;
    case '-':
      // ok
      break;
    case 'i':
      // If 'i' is the last character we only have an imaginary part.
      if (std::distance(it, end) == 1) {
        return {{0, re}, pending};
      }
      [[fallthrough]];  // fallthrough
    default:
      return {0, error::syntax};
    }

    // Read imaginary part.
    T im;
    std::tie(im, it, err) = parse_float_prefix<T>(it, end);
    if (err != nil) {
      if (err != error::range) {
        return {0, err};
      }
      pending = error::range;
    }
    if (std::distance(it, end) != 1 || *it != 'i') {
      return {0, error::syntax};
    }

    return {{re, im}, pending};
  }
};

}  // namespace bongo::strconv
