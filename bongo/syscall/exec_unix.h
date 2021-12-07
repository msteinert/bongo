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

inline std::pair<int, std::error_code> fcntl(int fd, int cmd, int arg) noexcept {
  auto val = ::fcntl(fd, cmd, arg);
  if (val == -1) {
    return {0, std::error_code{errno, std::system_category()}};
  }
  return {val, nil};
}

}  // namespace detail

inline std::pair<int, std::error_code> open(std::string const& path, int mode, uint32_t perm) noexcept {
  auto r0 = ::openat(AT_FDCWD, path.c_str(), mode|O_LARGEFILE, perm);
  if (r0 == -1) {
    return {-1, std::error_code{errno, std::system_category()}};
  }
  return {r0, nil};
}

inline std::error_code close(int fd) noexcept {
  auto r0 = ::close(fd);
  if (r0 == -1) {
    return std::error_code{errno, std::system_category()};
  }
  return nil;
}

inline std::error_code fstat(int fd, struct ::stat* s) {
  auto r0 = ::fstat(fd, s);
  if (r0 == -1) {
    return std::error_code{errno, std::system_category()};
  }
  return nil;
}

inline std::pair<int, std::error_code> close_on_exec(int fd) noexcept {
  return detail::fcntl(fd, F_SETFD, FD_CLOEXEC);
}

inline std::error_code set_nonblock(int fd, bool nonblocking) noexcept {
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

inline int getpagesize() noexcept {
  return ::sysconf(_SC_PAGESIZE);
}

inline void exit(int code) {
  return ::exit(code);
}

}  // namespace bongo::syscall
