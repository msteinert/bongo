// Copyright The Go Authors.

#pragma once

#include <system_error>

namespace bongo::detail::syscall::unix {

std::pair<bool, std::error_code> is_nonblock(int fd) noexcept;

}  // namespace bongo::detail::syscall::unix
