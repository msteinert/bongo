// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::context {

enum class error {
  canceled = 10,
  deadline_exceeded,
};

std::error_code make_error_code(error e);

}  // bongo::context

namespace std {

template <>
struct is_error_code_enum<bongo::context::error> : true_type {};

}  // namespace std
