// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#include <cxxabi.h>

#include <bongo/fmt/detail/concepts.h>

namespace bongo::fmt::detail {

inline auto demangle(char const* name) -> std::string {
  int status = 0;
  size_t size = 256;
  char buf[size];
  return abi::__cxa_demangle(name, buf, &size, &status);
}

template <typename T>
auto type_name(T const& v) -> std::string {
  return demangle((typeid (v)).name());
}

// too_large reports whether the magnitude of the integer is too large to be
// used as a formatting width or precision.
constexpr auto too_large(long x) -> bool {
  constexpr long max = 1e6;
  return x > max || x < -max;
}

// parse_number converts ASCII to an integer.
template <std::input_iterator It>
constexpr auto parse_number(It it, It end) -> std::tuple<long, bool, It> {
  if (it >= end) {
    return {0, false, it};
  }
  auto n = 0;
  auto is_num = false;
  for (; it < end && '0' <= *it && *it <= '9'; ++it) {
    if (too_large(n)){
      return {0, false, end};
    }
    n = n*10 + static_cast<long>(*it-'0');
    is_num = true;
  }
  return {n, is_num, it};
}

// parse_arg_number returns the value of the bracketed number, minus 1.
template <std::input_iterator It>
constexpr auto parse_arg_number(It begin, It end) -> std::tuple<long, bool, It> {
  if (std::distance(begin, end) < 3) {
    return {0, false, std::next(begin)};
  }
  auto it = begin = std::next(begin);
  for (; it < end; it = std::next(it)) {
    if (*it == ']') {
      auto [width, ok, new_it] = parse_number(begin, it);
      if (!ok || new_it != it) {
        return {0, false, std::next(it)};
      }
      return {width - 1, true, std::next(it)};
    }
  }
  return {0, false, it};
}

// int_from_arg gets the arg_num'th argument and reports if the argument has
// integer type.
template <typename... Args>
auto int_from_arg(long arg_num, Args... args) -> std::tuple<long, bool, long> {
  auto n = 0;
  auto is_int = false;
  auto new_arg_num = arg_num;
  if constexpr (sizeof... (args) > 0) {
    if (arg_num < static_cast<long>(sizeof... (args))) {
      visit_arg([&](auto& arg) {
        if constexpr (Integer<decltype(arg)>) {
          n = static_cast<long>(arg);
          is_int = true;
        } else {
          n = 0;
          is_int = false;
        }
      }, arg_num, args...);
    }
  }
  return {n, is_int, new_arg_num};
}

}  // namespace bongo::fmt::detail
