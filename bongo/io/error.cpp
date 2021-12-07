// Copyright The Go Authors.

#include <system_error>

#include "bongo/io/error.h"

namespace bongo::io {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "io"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::eof:
      return "EOF";
    case error::short_write:
      return "short write";
    case error::invalid_write:
      return "invalid write result";
    case error::short_buffer:
      return "short buffer";
    case error::unexpected_eof:
      return "unexpected EOF";
    case error::no_progress:
      return "multiple read calls returned no data or error";
    case error::whence:
      return "invalid whence";
    case error::offset:
      return "invalid offset";
    case error::closed_pipe:
      return "read/write on closed pipe";
    default:
      return "unrecognized error";
    }
  }
};

const error_category error_category{};

}  // namespace

std::error_code make_error_code(error e) {
  return {static_cast<int>(e), error_category};
}

}  // namespace bongo::io
