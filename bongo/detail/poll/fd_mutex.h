// Copyright The Go Authors.

#pragma once

#include <atomic>
#include <stdexcept>

#include <bongo/detail/semaphore.h>

namespace bongo::detail::poll {

class fd_mutex {
  std::atomic<uint64_t> state_ = 0;
  semaphore<1>          rsema_;
  semaphore<1>          wsema_;

 public:
  fd_mutex() = default;
  bool incref();
  bool incref_and_close();
  bool decref();
  bool rwlock(bool read);
  bool rwunlock(bool read);
};

constexpr static uint64_t mutex_closed   = 1llu << 0;
constexpr static uint64_t mutex_rlock    = 1llu << 1;
constexpr static uint64_t mutex_wlock    = 1llu << 2;
constexpr static uint64_t mutex_ref      = 1llu << 3;
constexpr static uint64_t mutex_ref_mask = ((1llu<<20) - 1) << 3;
constexpr static uint64_t mutex_rwait    = 1llu << 23;
constexpr static uint64_t mutex_rmask    = ((1llu<<20) - 1) << 23;
constexpr static uint64_t mutex_wwait    = 1llu << 43;
constexpr static uint64_t mutex_wmask    = ((1llu<<20) - 1) << 43;

struct overflow_error : std::runtime_error {
  overflow_error()
      : std::runtime_error{"too many concurrent operations on a single file or socket (max 1048575)"} {}
};

inline bool fd_mutex::incref() {
  for (;;) {
    auto old = state_.load();
    if ((old&mutex_closed) != 0) {
      return false;
    }
    auto next = old + mutex_ref;
    if ((next&mutex_ref_mask) == 0) {
      throw overflow_error{};
    }
    if (state_.compare_exchange_strong(old, next)) {
      return true;
    }
  }
}

inline bool fd_mutex::incref_and_close() {
  for (;;) {
    auto old = state_.load();
    if ((old&mutex_closed) != 0) {
      return false;
    }
    auto next = (old | mutex_closed) + mutex_ref;
    if ((next&mutex_ref_mask) == 0) {
      throw overflow_error{};
    }
    next &= ~(mutex_rmask | mutex_wmask);
    if (state_.compare_exchange_strong(old, next)) {
      while ((old&mutex_rmask) != 0) {
        old -= mutex_rwait;
        rsema_.release();
      }
      while ((old&mutex_wmask) != 0) {
        old -= mutex_wwait;
        wsema_.release();
      }
      return true;
    }
  }
}

inline bool fd_mutex::decref() {
  for (;;) {
    auto old = state_.load();
    if ((old&mutex_ref_mask) == 0) {
      throw std::logic_error{"inconsistent poll::fd_mutex"};
    }
    auto next = old - mutex_ref;
    if (state_.compare_exchange_strong(old, next)) {
      return (next&(mutex_closed|mutex_ref_mask)) == mutex_closed;
    }
  }
}

inline bool fd_mutex::rwlock(bool read) {
  uint64_t mutex_bit, mutex_wait, mutex_mask;
  semaphore<1>* mutex_sema;
  if (read) {
    mutex_bit = mutex_rlock;
    mutex_wait = mutex_rwait;
    mutex_mask = mutex_rmask;
    mutex_sema = &rsema_;
  } else {
    mutex_bit = mutex_wlock;
    mutex_wait = mutex_wwait;
    mutex_mask = mutex_wmask;
    mutex_sema = &wsema_;
  }
  for (;;) {
    auto old = state_.load();
    if ((old&mutex_closed) != 0) {
      return false;
    }
    uint64_t next = 0;
    if ((old&mutex_bit) == 0) {
      next = (old | mutex_bit) + mutex_ref;
      if ((next&mutex_ref_mask) == 0) {
        throw overflow_error{};
      }
    } else {
      next = old + mutex_wait;
      if ((next&mutex_mask) == 0) {
        throw overflow_error{};
      }
    }
    if (state_.compare_exchange_strong(old, next)) {
      if ((old&mutex_bit) == 0) {
        return true;
      }
      mutex_sema->acquire();
    }
  }
}

inline bool fd_mutex::rwunlock(bool read) {
  uint64_t mutex_bit, mutex_wait, mutex_mask;
  semaphore<1>* mutex_sema;
  if (read) {
    mutex_bit = mutex_rlock;
    mutex_wait = mutex_rwait;
    mutex_mask = mutex_rmask;
    mutex_sema = &rsema_;
  } else {
    mutex_bit = mutex_wlock;
    mutex_wait = mutex_wwait;
    mutex_mask = mutex_wmask;
    mutex_sema = &wsema_;
  }
  for (;;) {
    auto old = state_.load();
    if ((old&mutex_bit) == 0 || (old&mutex_ref_mask) == 0) {
      throw std::logic_error{"inconsistent pool::fd_mutex"};
    }
    auto next = (old & ~mutex_bit) - mutex_ref;
    if ((old&mutex_mask) != 0) {
      next -= mutex_wait;
    }
    if (state_.compare_exchange_strong(old, next)) {
      if ((old&mutex_mask) != 0) {
        mutex_sema->release();
      }
      return (next&(mutex_closed|mutex_ref_mask)) == mutex_closed;
    }
  }
}

}  // namespace bongo::detail::poll
