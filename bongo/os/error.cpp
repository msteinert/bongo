// Copyright The Go Authors.

#include <system_error>

#include "bongo/os/error.h"

namespace bongo::os {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "os"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
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

}  // namespace bongo::os
