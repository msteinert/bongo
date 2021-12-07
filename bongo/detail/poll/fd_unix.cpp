// Copyright The Go Authors.

#include <memory>
#include <string_view>
#include <system_error>

#include "bongo/bongo.h"
#include "bongo/detail/poll/error.h"
#include "bongo/detail/poll/fd_mutex.h"
#include "bongo/detail/poll/fd_unix.h"
#include "bongo/detail/poll/hook_unix.h"
#include "bongo/detail/semaphore.h"
#include "bongo/runtime/netpoll_epoll.h"
#include "bongo/syscall.h"

namespace bongo::detail::poll {
namespace {

template <typename Fn>
std::error_code ignoring_interrupted(Fn fn) {
  for (;;) {
    auto err = fn();
    if (err != std::errc::interrupted) {
      return err;
    }
  }
}

}  // namespace

std::error_code fd::init(std::string_view net, bool pollable) {
  if (net == "file") {
    is_file_ = true;
  }
  if (!pollable) {
    is_blocking_ = true;
    return nil;
  }
#if 0
  auto err = pd_.init(fd_);
  if (err) {
    // If the runtime poller could not be initialized assume blocking mode.
    is_blocking_ = true;
  }
  return err;
#endif
  return nil;
}

std::error_code fd::close() {
  if (!fdmu_.incref_and_close()) {
    return error_closing(is_file_);
  }
#if 0
  pd_.evict();
#endif
  auto err = decref();
  if (is_blocking_) {
    csema_.acquire();
  }
  return err;
}

std::error_code fd::destroy() {
#if 0
  pd_.close();
#endif
  auto err = close_func(sysfd);
  sysfd = -1;
  csema_.release();
  return err;
}

std::error_code fd::fstat(struct ::stat* s) {
  if (auto err = incref(); err) {
    return err;
  }
  auto _ = runtime::defer([this]() { decref(); });
  return ignoring_interrupted([&]() {
    return syscall::fstat(sysfd, s);
  });
}

std::error_code fd::incref() {
  if (!fdmu_.incref()) {
    return error_closing(is_file_);
  }
  return nil;
}

std::error_code fd::decref() {
  if (fdmu_.decref()) {
    return destroy();
  }
  return nil;
}

}  // namespace bongo::detail::poll
