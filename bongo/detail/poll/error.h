// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::detail::poll {

enum class error {
  file_closing = 10,
  net_closing,
};

std::error_code make_error_code(error e);

}  // namespace bongo::detail::poll

namespace std {

template <>
struct is_error_code_enum<bongo::detail::poll::error> : true_type {};

}  // namespace std

namespace bongo::detail::poll {

inline std::error_code error_closing(bool is_file) {
  if (is_file) {
    return error::file_closing;
  }
  return error::net_closing;
}

}  // namespace bongo::detail::poll
