// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::os {

enum class error {};

std::error_code make_error_code(error e);

}  // namespace bongo::os

namespace std {

template <>
struct is_error_code_enum<bongo::os::error> : true_type {};

}  // namespace std
