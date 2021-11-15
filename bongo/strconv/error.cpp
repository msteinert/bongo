// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <system_error>

#include "bongo/strconv/error.h"

namespace bongo::strconv {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "strconv"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::base:
      return "strconv base error";
    case error::range:
      return "strconv range error";
    case error::syntax:
      return "strconv syntax error";
    default:
      return "strconv unrecognized error";
    }
  }
};

const error_category error_category{};

}  // namespace

std::error_code make_error_code(error e) {
  return {static_cast<int>(e), error_category};
}

}  // namespace bongo::strconv
