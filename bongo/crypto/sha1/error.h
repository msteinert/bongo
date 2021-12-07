// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::crypto::sha1 {

enum class error {
  invalid_hash_state_identifier = 10,
  invalid_hash_state_size,
};

std::error_code make_error_code(error e);

}  // namespace bongo::crypto::sha1

namespace std {

template <>
struct is_error_code_enum<bongo::crypto::sha1::error> : true_type {};

}  // namespace std
