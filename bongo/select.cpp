// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <algorithm>
#include <atomic>
#include <mutex>
#include <optional>
#include <random>

#include "bongo/chan.h"
#include "bongo/detail/chan.h"
#include "bongo/error.h"
#include "bongo/select.h"

namespace bongo {
namespace {

std::mt19937& gen() {
  thread_local std::mt19937 val = std::mt19937{std::random_device{}()};
  return val;
}

template <typename T>
T randn(T n) {
  return std::uniform_int_distribution<T>{0, n}(gen());
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

void selunlock(select_case const* cases, size_t* lockorder, int n) noexcept {
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
  bool operator()(size_t left, size_t right) noexcept {
    return cases[left].chan < cases[right].chan;
  }
};

}  // namespace

size_t select(select_case const* cases, size_t size) {
  using size_type = size_t;

  if (size == 0) {
    detail::this_thread().forever_sleep();
  }

  size_type n = 0;
  std::optional<size_type> dflt;

  for (size_type i = 0; i < size; ++i) {
    switch (cases[i].direction) {
    case detail::select_default:
      if (dflt.has_value()) {
        throw logic_error{"multiple defaults in switch"};
      }
      dflt = i;
      continue;

    case detail::select_send:
    case detail::select_recv:
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
  size_type pollorder[n];
  std::fill(pollorder, pollorder + n, 0);
  size_type norder = 0;
  for (size_type i = 0; i < size; ++i) {
    if (cases[i].direction == detail::select_default || cases[i].chan == nullptr) {
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
    if (cas.direction == detail::select_send) {
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
    } else if (cas.direction == detail::select_recv) {
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
    case detail::select_send:
      c->sendq_.enqueue(t);
      break;
    case detail::select_recv:
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
      case detail::select_send:
        c->sendq_.dequeue(t);
        break;
      case detail::select_recv:
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
    if (selected_case->direction == detail::select_send) {
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
