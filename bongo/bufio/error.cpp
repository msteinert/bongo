// Copyright The Go Authors.

#include <system_error>

#include "bongo/bufio/error.h"

namespace bongo::bufio {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "bufio"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::invalid_unread_byte:
      return "invalid use of unread_byte";
    case error::invalid_unread_rune:
      return "invalid use of unread_rune";
    case error::buffer_full:
      return "buffer full";
    case error::negative_count:
      return "negative count";
    case error::negative_read:
      return "reader return negative count from read";
    case error::negative_write:
      return "write return negative count from write";
    case error::full_buffer:
      return "tried to fill full buffer";
    case error::rewind:
      return "tried to rewind past start of buffer";
    case error::token_too_long:
      return "scanner: token too long";
    case error::negative_advance:
      return "scanner: SplitFunction returns negative advance count";
    case error::advance_too_far:
      return "scanner: SplitFunction returns advance count beyond input";
    case error::bad_read_count:
      return "scanner: read returned impossible count";
    case error::final_token:
      return "final token";
    case error::buffer_after_scan:
      return "buffer called after scan";
    case error::too_many_empty_tokens:
      return "too many empty tokens without progressing";
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

}  // namespace bongo::bufio
