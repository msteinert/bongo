// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::io {

enum class error {
  eof = 1,
  short_write = 10,
  invalid_write,
  short_buffer,
  unexpected_eof,
  no_progress,
  whence,
  offset,
  closed_pipe,
};

std::error_code make_error_code(error e);

constexpr error eof = error::eof;

}  // namespace bongo::io

namespace std {

template <>
struct is_error_code_enum<bongo::io::error> : true_type {};

}  // namespace std
