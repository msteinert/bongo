// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::bufio {

enum class error {
  invalid_unread_byte = 10,
  invalid_unread_rune,
  buffer_full,
  negative_count,
  negative_read,
  negative_write,
  full_buffer,
  rewind,
  token_too_long,
  negative_advance,
  advance_too_far,
  bad_read_count,
  final_token,
  buffer_after_scan,
  too_many_empty_tokens,
};

std::error_code make_error_code(error e);

}  // namespace bongo::bufio

namespace std {

template <>
struct is_error_code_enum<bongo::bufio::error> : true_type {};

}  // namespace std
