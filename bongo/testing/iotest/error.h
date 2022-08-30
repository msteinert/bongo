// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::testing::iotest {

enum class error {
  timeout = 10,
};

std::error_code make_error_code(error e);

}  // namespace bongo::testing::iotest

namespace std {

template <>
struct is_error_code_enum<bongo::testing::iotest::error> : true_type {};

}  // namespace std
