// Copyright The Go Authors.

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <iterator>
#include <mutex>

namespace bongo::detail {

using select_value = void*;

enum select_direction {
  select_send,
  select_recv,
  select_default,
};

// Represents an OS thread.
struct thread {
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic_bool select_done_ = false;

  // Block this thread forever.
  void forever_sleep();
};

thread& this_thread();

struct waitq {
  // Represents a thread in a wait queue.
  struct thread {
    detail::thread& parent_ = this_thread();
    thread* next_ = nullptr;
    thread* prev_ = nullptr;
    bool done_waiting_ = false;
    bool closed_ = false;
    select_value value_ = nullptr;
    bool is_select_ = false;
  };

  std::atomic<thread*> first_ = nullptr;
  thread* last_ = nullptr;

  void enqueue(thread* t) noexcept;
  thread* dequeue() noexcept;
  void dequeue(thread* t) noexcept;
};

struct chan_impl {
  waitq sendq_;
  waitq recvq_;
  size_t const size_;
  std::atomic_size_t count_ = 0;
  size_t sendx_ = 0;
  size_t recvx_ = 0;
  std::atomic_bool closed_ = false;
  std::mutex mutex_;

  chan_impl(size_t size)
      : size_{size} {}

  virtual ~chan_impl() {}
  virtual void reset(select_value value_ptr) noexcept = 0;
  virtual void send(select_value from_ptr, waitq::thread* t) noexcept = 0;
  virtual void send(select_value from_ptr) noexcept = 0;
  virtual void recv(select_value to_ptr, waitq::thread* t) noexcept = 0;
  virtual void recv(select_value to_ptr) noexcept = 0;
};

}  // namespace bongo::detail
