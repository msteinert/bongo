// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <atomic>
#include <mutex>

#include "bongo/detail/chan.h"
#include "bongo/error.h"

namespace bongo::detail {

thread& this_thread() {
  thread_local thread t{};
  return t;
}

void detail::thread::forever_sleep() {
  auto& t = detail::this_thread();
  std::unique_lock lock{t.mutex_};
  t.cond_.wait(lock, []() { return false; });
  throw logic_error{"unreachable"};
}

void waitq::enqueue(waitq::thread* t) noexcept {
  if (!last_) {
    first_.store(t, std::memory_order_relaxed);
  } else {
    t->prev_ = last_;
    last_->next_ = t;
  }
  last_ = t;
}

waitq::thread* waitq::dequeue() noexcept {
  while (true) {
    auto t = first_.load(std::memory_order_relaxed);
    if (t) {
      if (!t->next_) {
        first_.store(nullptr, std::memory_order_relaxed);
        last_ = nullptr;
      } else {
        t->next_->prev_ = nullptr;
        first_.store(t->next_, std::memory_order_relaxed);
        t->next_ = nullptr;
      }
      if (t->is_select_) {
        bool exp = false;
        if (!t->parent_.select_done_.compare_exchange_strong(exp, true)) {
          continue;
        }
      }
    }
    return t;
  }
}

void waitq::dequeue(waitq::thread* t) noexcept {
  auto prev = t->prev_;
  auto next = t->next_;
  if (prev) {
    if (next) {
      // Middle of queue
      prev->next_ = next;
      next->prev_ = prev;
      t->next_ = nullptr;
      t->prev_ = nullptr;
    } else {
      // End of queue
      prev->next_ = nullptr;
      last_ = prev;
      t->prev_ = nullptr;
    }
  } else {
    if (next) {
      // Start of queue
      next->prev_ = nullptr;
      first_.store(next, std::memory_order_relaxed);
      t->next_ = nullptr;
    } else {
      // Only element or already removed
      if (first_.load(std::memory_order_relaxed) == t) {
        first_.store(nullptr, std::memory_order_relaxed);
        last_ = nullptr;
      }
    }
  }
}

}  // namespace bongo::detail
