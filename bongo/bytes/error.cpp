// Copyright The Go Authors.

#include <system_error>

#include "bongo/bytes/error.h"

namespace bongo::bytes {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "bytes"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::unread_byte:
      return "unread_byte: previous operation was not a successful read";
    case error::unread_rune:
      return "unread_rune: previous operation was not a successful read_rune";
    case error::at_beginning:
      return "at beginning of span";
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

}  // namespace bongo::bytes
