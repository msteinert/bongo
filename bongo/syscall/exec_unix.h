// Copyright The Go Authors.

#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <system_error>

#include <bongo/bongo.h>

namespace bongo::syscall {
namespace detail {

inline auto fcntl(int fd, int cmd, int arg) noexcept -> std::pair<int, std::error_code> {
  auto val = ::fcntl(fd, cmd, arg);
  if (val == -1) {
    return {0, std::error_code{errno, std::system_category()}};
  }
  return {val, nil};
}

}  // namespace detail

inline auto open(std::string const& path, int mode, uint32_t perm) noexcept -> std::pair<int, std::error_code> {
  auto r0 = ::openat(AT_FDCWD, path.c_str(), mode, perm);
  if (r0 == -1) {
    return {-1, std::error_code{errno, std::system_category()}};
  }
  return {r0, nil};
}

inline auto close(int fd) noexcept -> std::error_code {
  auto r0 = ::close(fd);
  if (r0 == -1) {
    return std::error_code{errno, std::system_category()};
  }
  return nil;
}

inline auto fstat(int fd, struct ::stat* s) -> std::error_code {
  auto r0 = ::fstat(fd, s);
  if (r0 == -1) {
    return std::error_code{errno, std::system_category()};
  }
  return nil;
}

inline auto close_on_exec(int fd) noexcept -> std::pair<int, std::error_code> {
  return detail::fcntl(fd, F_SETFD, FD_CLOEXEC);
}

inline auto set_nonblock(int fd, bool nonblocking) noexcept -> std::error_code {
  auto [flag, err] = detail::fcntl(fd, F_GETFL, 0);
  if (err) {
    return err;
  }
  if (nonblocking) {
    flag |= O_NONBLOCK;
  } else {
    flag &= ~O_NONBLOCK;
  }
  std::tie(flag, err) = detail::fcntl(fd, F_SETFL, flag);
  if (err) {
    return err;
  }
  return nil;
}

inline auto getpagesize() noexcept -> int {
  return ::sysconf(_SC_PAGESIZE);
}

[[noreturn]] inline void exit(int code) noexcept {
  ::exit(code);
}

}  // namespace bongo::syscall
