// Copyright The Go Authors.

#include <system_error>

#include "bongo/strings/error.h"

namespace bongo::strings {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "strings"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::at_beginning:
      return "at beginning of string";
    case error::prev_read_rune:
      return "previous operation was not read_rune";
    case error::negative_position:
      return "negative position";
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

}  // namespace bongo::strings
