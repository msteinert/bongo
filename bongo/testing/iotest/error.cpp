// Copyright The Go Authors.

#include <system_error>

#include "bongo/testing/iotest/error.h"

namespace bongo::testing::iotest {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "testing/iotest"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::timeout:
      return "timeout";
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

}  // namespace bongo::testing::iotest
