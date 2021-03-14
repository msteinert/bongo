// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <stdexcept>

namespace bongo {

struct logic_error : public std::logic_error {
  using std::logic_error::logic_error;
};

}  // namespace bongo
