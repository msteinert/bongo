// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <cstddef>
#include <exception>

namespace bongo {

struct error : public std::exception {
  error() = default;
  error(std::nullptr_t) {}
  virtual bool operator==(std::nullptr_t) const noexcept { return true; }
  virtual bool operator==(bool b) const noexcept { return b == false; }
  virtual operator bool() const noexcept { return false; }
  char const* what() const noexcept override { std::terminate(); }
};

class runtime_error : public error {
  std::string what_;
 public:
  runtime_error(char const* s)
      : what_{s} {}
  runtime_error(std::string const& s)
      : what_{s} {}
  runtime_error(std::string&& s)
      : what_{std::move(s)} {}
  runtime_error(std::string_view s)
      : what_{s} {}
  bool operator==(std::nullptr_t) const noexcept final { return false; }
  bool operator==(bool b) const noexcept final { return b == true; }
  operator bool() const noexcept final { return true; }
  char const* what() const noexcept final { return what_.c_str(); }
};

struct logic_error : public runtime_error { using runtime_error::runtime_error; };

}  // namespace bongo
