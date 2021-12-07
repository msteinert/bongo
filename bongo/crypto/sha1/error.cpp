// Copyright The Go Authors.

#include <system_error>

#include "bongo/crypto/sha1/error.h"

namespace bongo::crypto::sha1 {
namespace {

struct error_category : std::error_category {
  const char* name() const noexcept { return "crypto/sha1"; }
  std::string message(int e) const {
    switch (static_cast<error>(e)) {
    case error::invalid_hash_state_identifier:
      return "invalid hash state identifier";
    case error::invalid_hash_state_size:
      return "invalid hash state size";
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

}  // namespace bongo::crypto::sha1
