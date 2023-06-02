// Copyright The Go Authors.

#include "bongo/flag/error.h"

#include <system_error>

namespace bongo::flag {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "flag"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::flag:
      return "flag: help requested";
    case error::parse:
      return "parse error";
    case error::range:
      return "value out of range";
    default:
      return "unrecognized error";
    }
  }
};

const error_category error_category{};

}  // namespace

std::error_code make_error_code(error e) { return {static_cast<int>(e), error_category}; }

}  // namespace bongo::flag
