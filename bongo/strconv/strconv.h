// Copyright The Go Authors.

#pragma once

#include <complex>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/strconv/atob.h>
#include <bongo/strconv/atoc.h>
#include <bongo/strconv/atof.h>
#include <bongo/strconv/atoi.h>
#include <bongo/strconv/btoa.h>
#include <bongo/strconv/ctoa.h>
#include <bongo/strconv/ftoa.h>
#include <bongo/strconv/itoa.h>

namespace bongo::strconv {

/// Parse with default parameters.
template <typename T, std::input_iterator InputIt>
constexpr auto parse(InputIt begin, InputIt end) -> std::pair<T, std::error_code> {
  return parser<T, InputIt>{}(begin, end);
}

/// Integer with base parameter.
template <typename T, std::input_iterator InputIt>
constexpr auto parse(InputIt begin, InputIt end, int base) -> std::pair<T, std::error_code> {
  return parser<T, InputIt>{}(begin, end, base);
}

/// Parse anything with default parameters.
template <typename T>
constexpr auto parse(std::string_view s) -> std::pair<T, std::error_code> {
  return parser<T, std::string_view::iterator>{}(std::begin(s), std::end(s));
}

/// Integer with base parameter.
template <typename T>
constexpr auto parse(std::string_view s, int base) -> std::pair<T, std::error_code> {
  return parser<T, std::string_view::iterator>{}(std::begin(s), std::end(s), base);
}

/// Format with default parameters.
template <typename T>
auto format(T v) -> std::string {
  std::string out;
  format(v, std::back_inserter(out));
  return out;
}

/// Integer with base parameter.
template <typename T>
auto format(T v, int arg0) -> std::string {
  std::string out;
  format(v, std::back_inserter(out), arg0);
  return out;
}

/// Floating point with format and precision parameters.
template <typename T>
auto format(T v, char arg0, int arg1) -> std::string {
  std::string out;
  format(v, std::back_inserter(out), arg0, arg1);
  return out;
}

template <typename T>
auto format(std::complex<T> v, char arg0, int arg1) -> std::string {
  std::string out;
  format(v, std::back_inserter(out), arg0, arg1);
  return out;
}

/// Floating point with format parameter.
template <typename T>
auto format(T v, char arg0) -> std::string {
  std::string out;
  format(v, std::back_inserter(out), arg0);
  return out;
}

template <typename T>
auto format(std::complex<T> v, char arg0) -> std::string {
  std::string out;
  format(v, std::back_inserter(out), arg0);
  return out;
}

}  // namespace bongo::strconv
