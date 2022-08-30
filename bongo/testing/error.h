// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::testing {

enum class error {
  test_error = 10,
};

std::error_code make_error_code(error e);

}  // namespace bongo::testing

namespace std {

template <>
struct is_error_code_enum<bongo::testing::error> : true_type {};

}  // namespace std
