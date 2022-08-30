// Copyright The Go Authors.

#include <system_error>

#include "bongo/testing/error.h"

namespace bongo::testing {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "testing"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::test_error:
      return "test error";
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

}  // namespace bongo::testing
