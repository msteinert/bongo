// Copyright The Go Authors.

#pragma once

#include <sys/epoll.h>

namespace bongo::runtime {

class netpoll {
  int epfd_ = -1;

 public:
  netpoll() {
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epfd_ == -1) {
      epfd_ = epoll_create(1024);
      if (epfd_ == -1) {
        throw std::runtime_error{"runtime: epoll_create failed with " + std::to_string(-epfd_)};
      }
    }
  }

  ~netpoll() {
    ::close(epfd_);
  }

  int close(uintptr_t fd) {
    auto ev = epoll_event{};
    return -epoll_ctl(epfd_, EPOLL_CTL_DEL, static_cast<int>(fd), &ev);
  }
};

}  // namesapce bongo::runtime
