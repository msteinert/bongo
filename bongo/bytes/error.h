// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::bytes {

enum class error {
  unread_byte = 10,
  unread_rune,
  at_beginning,
  prev_read_rune,
  negative_position,
};

std::error_code make_error_code(error e);

}  // namespace bongo::bytes

namespace std {

template <>
struct is_error_code_enum<bongo::bytes::error> : true_type {};

}  // namespace std
