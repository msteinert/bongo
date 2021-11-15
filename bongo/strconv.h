// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <charconv>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <bongo/error.h>
#include <bongo/detail/type_traits.h>

namespace bongo::strconv {

template <typename T, typename U = void>
struct parser;

template <typename T, typename U = void>
struct formatter;

template <>
struct parser<bool> {
  std::pair<bool, error> operator()(std::string_view s) {
    if (s == "1" || s == "t" || s == "T" || s == "true" || s == "TRUE" || s == "True") {
      return {true, nullptr};
    }
    if (s == "0" || s == "f" || s == "F" || s == "false" || s == "FALSE" || s == "False") {
      return {false, nullptr};
    }
    return {false, runtime_error{"blah"}};
  }
};

template <>
struct formatter<bool> {
  std::string operator()(bool v) {
    return v ? "true" : "false";
  }
};

template <typename T>
struct parser<T, std::enable_if_t<std::is_integral_v<T>>> {
  std::pair<T, error> operator()(std::string_view s, int base = 10) {
    T value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value, base);
    if (ec == std::errc{}) {
      if (ptr != s.end()) {
        return {0, runtime_error{"blah"}};
      }
      return {value, nullptr};
    } else if (ec == std::errc::invalid_argument) {
      return {0, runtime_error{"blah"}};
    } else if (ec == std::errc::result_out_of_range) {
      return {0, runtime_error{"blah"}};
    } else {
      return {0, runtime_error{"blah"}};
    }
  }
};

template <typename T>
struct formatter<T, std::enable_if_t<std::is_integral_v<T>>> {
  std::string operator()(T v) {
    std::array<char, 32> s;
    auto [ptr, ec] = std::to_chars(s.data(), s.data() + s.size(), v);
    if (ec == std::errc{}) {
      return std::string{s.data(), ptr};
    } else {
      return {0, runtime_error{"blah"}};
    }
  }
};

template <typename T>
std::pair<T, error> parse(std::string_view s) {
  return parser<T>{}(s);
}

template <typename T>
std::pair<T, error> parse(std::string_view s, int base) {
  return parser<T>{}(s, base);
}

template <typename T>
std::string format(T&& v) {
  return formatter<T>{}(std::forward<T>(v));
}

}  // namespace bongo::strconv
