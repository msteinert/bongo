// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <istream>
#include <type_traits>

namespace bongo::detail {

static constexpr bool host_32bit = sizeof (void*) == 4;
static constexpr bool host_64bit = sizeof (void*) == 8;

}  // namespace bongo::detail
