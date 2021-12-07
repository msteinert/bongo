// Copyright The Go Authors.

#pragma once

#include <system_error>

#include <bongo/syscall.h>

namespace bongo::detail::poll {

static inline std::error_code(*close_func)(int) = syscall::close;

}  // namespace bongo::detail::poll
