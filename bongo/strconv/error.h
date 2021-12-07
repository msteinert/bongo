// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::strconv {

enum class error {
  base = 10,
  range,
  syntax,
};

std::error_code make_error_code(error e);

}  // namespace bongo::strconv

namespace std {

template <>
struct is_error_code_enum<bongo::strconv::error> : true_type {};

}  // namespace std
