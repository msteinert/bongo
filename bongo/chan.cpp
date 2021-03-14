// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <algorithm>
#include <atomic>
#include <mutex>
#include <optional>
#include <random>

#include "bongo/chan.h"
#include "bongo/error.h"

namespace bongo {
namespace {

thread_local std::mt19937 gen = std::mt19937{std::random_device{}()};

template <typename T>
T randn(T n) {
  return std::uniform_int_distribution<T>{0, n}(gen);
}

void sellock(select_case const* cases, size_t* lockorder, size_t n) noexcept {
  detail::chan* c = nullptr;
  for (size_t i = 0; i < n; ++i) {
    auto* c0 = cases[lockorder[i]].chan;
    if (c0 != c) {
      c = c0;
      c->mutex_.lock();
    }
  }
}

void selunlock(select_case const* cases, size_t* lockorder, size_t n) noexcept {
  for (int i = n - 1; i >= 0; --i) {
    auto* c = cases[lockorder[i]].chan;
    if (i > 0 && c == cases[lockorder[i - 1]].chan) {
      continue;  // Unlock on next iteration
    }
    c->mutex_.unlock();
  }
}

struct cmp {
  select_case const* cases;
  bool operator()(size_t left, size_t right) {
    return cases[left].chan < cases[right].chan;
  }
};

}  // namespace

namespace detail {

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

}  // namespace detail

size_t select(select_case const* cases, size_t size) {
  using size_type = size_t;

  if (size == 0) {
    detail::this_thread().forever_sleep();
  }

  size_type n = 0;
  std::optional<size_type> dflt;

  for (size_type i = 0; i < size; ++i) {
    switch (cases[i].direction) {
    case select_default:
      if (dflt.has_value()) {
        throw logic_error{"multiple defaults in switch"};
      }
      dflt = i;
      continue;

    case select_send:
    case select_recv:
      if (cases[i].chan == nullptr) {
        continue;
      }
      ++n;
      break;

    default:
      throw logic_error{"invalid select case"};
    }
  }

  if (n == 0) {
    if (!dflt.has_value()) {
      detail::this_thread().forever_sleep();
    }
    return dflt.value();
  }

  // Generate poll order
  size_type pollorder[n]{0};
  size_type norder = 0;
  for (size_type i = 0; i < size; ++i) {
    if (cases[i].direction == select_default || cases[i].chan == nullptr) {
      continue;
    }
    auto j = randn(norder);
    pollorder[norder] = pollorder[j];
    pollorder[j] = i;
    ++norder;
  }

  // Generate lock order
  size_type lockorder[n];
  for (size_type i = 0; i < n; ++i) {
    lockorder[i] = pollorder[i];
  }
  std::make_heap(lockorder, lockorder + n, cmp{cases});
  std::sort_heap(lockorder, lockorder + n, cmp{cases});

  // Lock all channels in select
  sellock(cases, lockorder, n);

  // Pass 1 - look for something already waiting
  for (auto i : pollorder) {
    auto& cas = cases[i];
    auto* c = cas.chan;
    if (cas.direction == select_send) {
      if (c->closed_.load(std::memory_order_relaxed)) {
        selunlock(cases, lockorder, n);
        throw logic_error{"send on closed channel"};
      }
      auto* t = c->recvq_.dequeue();
      if (t) {
        c->send(cas.value, t);
        selunlock(cases, lockorder, n);
        return i;
      }
      if (c->count_.load(std::memory_order_relaxed) < c->size_) {
        c->send(cas.value);
        selunlock(cases, lockorder, n);
        return i;
      }
    } else if (cas.direction == select_recv) {
      auto *t = c->sendq_.dequeue();
      if (t) {
        c->recv(cas.value, t);
        selunlock(cases, lockorder, n);
        return i;
      }
      if (c->count_.load(std::memory_order_relaxed) > 0) {
        c->recv(cas.value);
        selunlock(cases, lockorder, n);
        return i;
      }
      if (c->closed_.load(std::memory_order_relaxed)) {
        c->reset(cas.value);
        selunlock(cases, lockorder, n);
        return i;
      }
    } else {
      selunlock(cases, lockorder, n);
      throw logic_error{"unreachable"};
    }
  }

  if (dflt) {
    // Unlock all channels
    selunlock(cases, lockorder, n);
    return dflt.value();
  }

  // Pass 2 - enqueue on all channels
  detail::waitq::thread threads[n];

  for (size_type i = 0; i < n; ++i) {
    auto& cas = cases[lockorder[i]];
    auto* c = cas.chan;
    auto* t = &threads[i];
    t->value_ = cas.value;
    t->is_select_ = true;

    switch (cas.direction) {
    case select_send:
      c->sendq_.enqueue(t);
      break;
    case select_recv:
      c->recvq_.enqueue(t);
      break;
    default:
      selunlock(cases, lockorder, n);
      throw logic_error{"unreachable"};
    }
  }

  // Wait for somebody to wake us up
  auto& this_thread = detail::this_thread();
  std::unique_lock lock{this_thread.mutex_};
  this_thread.select_done_.store(false, std::memory_order_relaxed);
  selunlock(cases, lockorder, n);
  this_thread.cond_.wait(lock, [&this_thread]() {
    return this_thread.select_done_.load(std::memory_order_relaxed);
  });
  lock.unlock();

  // Pass 3 - dequeue from unsuccessful channels
  size_type casei = 0;
  sellock(cases, lockorder, n);
  select_case const* selected_case = nullptr;
  detail::waitq::thread* selected_thread = nullptr;

  for (size_type i = 0; i < n; ++i) {
    auto* t = &threads[i];
    if (t->done_waiting_) {
      casei = lockorder[i];
      selected_case = &cases[lockorder[i]];
      selected_thread = t;
    } else {
      auto& cas = cases[lockorder[i]];
      auto* c = cas.chan;

      switch (cas.direction) {
      case select_send:
        c->sendq_.dequeue(t);
        break;
      case select_recv:
        c->recvq_.dequeue(t);
        break;
      default:
        selunlock(cases, lockorder, n);
        throw logic_error{"unreachable"};
      }
    }
  }

  if (!selected_case) {
    selunlock(cases, lockorder, n);
    throw logic_error{"bad wakeup"};
  } else {
    if (selected_case->direction == select_send) {
      if (selected_thread->closed_) {
        selunlock(cases, lockorder, n);
        throw logic_error{"send on closed channel"};
      }
    }
  }

  // Success
  selunlock(cases, lockorder, n);
  return casei;
}

}  // namespace bongo
