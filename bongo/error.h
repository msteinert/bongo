// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <stdexcept>

namespace bongo {

struct error : public std::runtime_error { using std::runtime_error::runtime_error; };
struct logic_error : public error { using error::error; };

}  // namespace bongo
