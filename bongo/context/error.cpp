// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <system_error>

#include "bongo/context/error.h"

namespace bongo::context {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "context"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::canceled:
      return "context canceled";
    case error::deadline_exceeded:
      return "context deadline exceeded";
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

}  // namespace bongo::context
