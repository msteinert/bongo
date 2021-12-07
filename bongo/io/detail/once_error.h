// Copyright The Go Authors.

#pragma once

#include <mutex>
#include <system_error>

namespace bongo::io::detail {

class once_error {
  std::mutex mutex_;
  std::error_code err_ = nil;

 public:
  void store(std::error_code err) {
    auto lock = std::lock_guard{mutex_};
    if (err_ != nil) {
      return;
    }
    err_ = err;
  }

  std::error_code load() {
    auto lock = std::lock_guard{mutex_};
    return err_;
  }
};

}  // namespace bongo::io::detail
