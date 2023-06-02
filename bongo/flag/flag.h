// Copyright The Go Authors.

#pragma once

#include <bongo/bongo.h>
#include <bongo/flag/error.h>
#include <bongo/io.h>
#include <bongo/strconv.h>

#include <concepts>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace bongo::flag {

template <typename T, typename U>
concept Value = requires(T v, U u, std::string_view s) {
  { v.str() } -> std::same_as<std::string>;
  { v.set(s) } -> std::same_as<std::error_code>;
  { v.get() } -> std::same_as<U const&>;
};

template <typename T>
auto str(T& v) -> std::string {
  return v.str();
};

template <typename T>
auto set(T& v, std::string_view s) -> std::error_code {
  return v.set(s);
}

template <typename T, typename U>
auto get(T& v) -> U const& {
  return v.get();
}

template <typename T, typename U>
concept ValueFunc = requires(T v, std::string_view s) {
  { str(v) } -> std::same_as<std::error_code>;
  { set(v, s) } -> std::same_as<std::error_code>;
  { get(v) } -> std::same_as<U const&>;
};

template <typename T>
concept Bool = std::same_as < std::remove_cvref_t<T>,
bool > ;

template <typename T>
concept Integer = std::integral<std::remove_cvref_t<T>> && !std::same_as<std::remove_cvref_t<T>, bool>;

template <typename T>
concept Floating = std::same_as < std::remove_cvref_t<T>,
float > || std::same_as<std::remove_cvref_t<T>, double>;

template <typename T>
concept Numeric = Integer<T> || Floating<T>;

template <typename T>
class value;

// Boolean value
template <Bool T>
class value<T> {
  T v_;

 public:
  value(T v)
      : v_{v} {}

  auto str() -> std::string { return strconv::format(v_); }

  auto set(std::string_view s) -> std::error_code {
    auto [v, err] = strconv::parse<T>(s);
    if (err != nil) {
      err = error::parse;
    }
    v_ = v;
    return err;
  }

  auto get() -> T const& { return v_; }
};

// Numeric value
template <typename T>
requires Numeric<T>
class value<T> {
  T v_;

 public:
  value(T v)
      : v_{v} {}

  auto str() -> std::string { return strconv::format(v_); }

  auto set(std::string_view s) -> std::error_code {
    auto [v, err] = strconv::parse<T>(s);
    if (err != nil) {
      switch (err) {
      case strconv::error::syntax:
        err = error::parse;
        break;
      case strconv::error::range:
        err = error::parse;
        break;
      default:
        break;
      }
    }
    v_ = v;
    return err;
  }

  auto get() -> T const& { return v_; }
};

enum class error_handling {
  continue_on_error,
  exit_on_error,
  panic_on_error,
};

class flag_set {
  std::string name_;
  error_handling error_handling_;

 public:
  flag_set(std::string name, error_handling e)
      : name_{std::move(name)}
      , error_handling_{e} {}
  flag_set(std::string name)
      : name_{std::move(name)}
      , error_handling_{error_handling::continue_on_error} {}

  auto parse(std::span<char const* const*> /* arguments */) -> std::error_code { return nil; }
  auto parse(std::span<std::string_view> /* arguments */) -> std::error_code { return nil; }
  auto parse(std::span<std::string const> /* arguments */) -> std::error_code { return nil; }
};

}  // namespace bongo::flag
