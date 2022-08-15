// Copyright The Go Authors.

#pragma once

#include <concepts>
#include <string>

namespace bongo::fmt {

// Stringer is implemented by any value that has a str method, which defines
// the ``native'' format for that value.
template <typename T>
concept Stringer = requires (T s) {
  { s.str() } -> std::convertible_to<std::string_view>;
};

template <typename T> requires Stringer<T>
auto to_string(T& v) {
  return v.str();
}

template <typename T>
concept StringerFunc = requires (T s) {
  { to_string(s) } -> std::convertible_to<std::string_view>;
};

// BongoStringer is implemented by any value that has a bongostr method, which
// defines the bongo syntax for that value.
template <typename T>
concept BongoStringer = requires (T s) {
  { s.bongostr() } -> std::convertible_to<std::string_view>;
};

template <typename T> requires BongoStringer<T>
auto to_bongo_string(T& v) {
  return v.bongostr();
}

template <typename T>
concept BongoStringerFunc = requires (T s) {
  { to_bongo_string(s) } -> std::convertible_to<std::string_view>;
};

}  // namespace bongo::fmt
