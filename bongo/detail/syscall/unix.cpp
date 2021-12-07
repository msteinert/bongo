// Copyright The Go Authors.

#include <unistd.h>
#include <fcntl.h>

#include <system_error>

#include "bongo/bongo.h"
#include "bongo/detail/syscall/unix.h"

namespace bongo::detail::syscall::unix {

std::pair<bool, std::error_code> is_nonblock(int fd) noexcept {
  auto flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return {false, std::error_code{errno, std::system_category()}};
  }
  return {flags&O_NONBLOCK, nil};
}

}  // namespace bongo::detail::syscall::unix
