// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <memory>
#include <mutex>
#include <stdexcept>

#include "bongo/sync.h"

namespace bongo::sync {

void wait_group::add(int n) {
  std::unique_lock lock{mutex_};
  state_ += n;
  if (state_ < 0) {
    throw std::logic_error{"negative counter"};
  }
  if (state_ == 0) {
    lock.unlock();
    cond_.notify_all();
  }
}

void wait_group::done() {
  add(-1);
}

void wait_group::wait() {
  std::unique_lock lock{mutex_};
  if (state_ > 0) {
    cond_.wait(lock, [this]() { return state_ == 0; });
  }
}

}  // namesapce bongo::sync
