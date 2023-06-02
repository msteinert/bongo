// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::flag {

enum class error {
  help = 10,
  flag,
  parse,
  range,
};

std::error_code make_error_code(error e);

}  // namespace bongo::flag

namespace std {

template <>
struct is_error_code_enum<bongo::flag::error> : true_type {};

}  // namespace std
