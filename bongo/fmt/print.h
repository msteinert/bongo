// Copyright The Go Authors.

#pragma once

#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/fmt/detail/printer.h>
#include <bongo/io.h>
#include <bongo/os.h>

namespace bongo::fmt {

// fprintf formats according to a format specifier and writes to w. It returns
// the number of bytes written and any write error encountered.
template <typename T, typename... Args> requires io::Writer<T>
auto fprintf(T& w, std::string_view format, Args&&... args) -> std::pair<long, std::error_code> {
  using io::write;
  auto p = detail::printer{};
  p.do_printf(format, std::forward<Args>(args)...);
  return write(w, p.bytes());
}

// fprintf formats according to a format specifier and writes to standard
// output. It returns the number of bytes written and any write error
// encountered.
template <typename... Args>
auto printf(std::string_view format, Args&&... args) -> std::pair<long, std::error_code> {
  return fprintf(os::stdout, format, std::forward<Args>(args)...);
}

// sprintf formats according to a format specifier and returns the resulting
// string.
template <typename... Args>
auto sprintf(std::string_view fmt, Args&&... args) -> std::string {
  auto p = detail::printer{};
  p.do_printf(fmt, std::forward<Args>(args)...);
  return std::string{p.str()};
}

// fprint formats using the default formats for its operands and writes to w.
// Spaces are added between operands when neither is a string. It returns the
// number of bytes written and any write error encountered.
template <typename T, typename... Args> requires io::Writer<T>
auto fprint(T& w, Args&&... args) -> std::pair<long, std::error_code> {
  using io::write;
  auto p = detail::printer{};
  p.do_print(std::forward<Args>(args)...);
  return write(w, p.bytes());
}

// fprint formats using the default formats for its operands and writes to
// standard out. Spaces are added between operands when neither is a string.
// It returns the number of bytes written and any write error encountered.
template <typename... Args>
auto print(Args&&... args) -> std::pair<long, std::error_code> {
  return fprint(os::stdout, std::forward<Args>(args)...);
}

// fprint formats using the default formats for its operands and returns the
// resulting string. Spaces are added between operands when neither is a
// string.
template <typename... Args>
auto sprint(Args&&... args) -> std::string {
  if constexpr (sizeof... (args) > 0) {
    auto p = detail::printer{};
    p.do_print(std::forward<Args>(args)...);
    return std::string{p.str()};
  }
  return std::string{};
}

// fprintln formats using the default formats for its operands and writes to w.
// Spaces are always added between operands and a newline is appended. It
// returns the number of bytes written and any write error encountered.
template <typename T, typename... Args> requires io::Writer<T>
auto fprintln(T& w, Args&&... args) -> std::pair<long, std::error_code> {
  using io::write;
  auto p = detail::printer{};
  p.do_println(std::forward<Args>(args)...);
  return write(w, p.bytes());
}

// println formats using the default formats for its operands and writes to
// standard output. Spaces are always added between operands and a newline is
// appended. It returns the number of bytes written and any write error
// encountered.
template <typename... Args>
auto println(Args&&... args) -> std::pair<long, std::error_code> {
  return fprintln(os::stdout, std::forward<Args>(args)...);
}

// sprintln formats using the default formats for its operands and returns the
// resulting string. Spaces are always added between operands and a newline is
// appended.
template <typename... Args>
auto sprintln(Args&&... args) -> std::string {
  if constexpr (sizeof... (args) > 0) {
    auto p = detail::printer{};
    p.do_println(std::forward<Args>(args)...);
    return std::string{p.str()};
  }
  return std::string{"\n"};
}

}  // namespace bongo::fmt
