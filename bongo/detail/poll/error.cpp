// Copyright The Go Authors.

#include <system_error>

#include "bongo/detail/poll/error.h"

namespace bongo::detail::poll {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "detail/poll"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::file_closing:
      return "use of closed file";
    case error::net_closing:
      return "use of closed network connection";
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

}  // namespace bongo::detail::poll
