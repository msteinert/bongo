// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::strings {

enum class error {
  at_beginning = 10,
  prev_read_rune,
  negative_position,
};

std::error_code make_error_code(error e);

}  // namespace bongo::strings

namespace std {

template <>
struct is_error_code_enum<bongo::strings::error> : true_type {};

}  // namespace std
