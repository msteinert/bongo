// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <bongo/error.h>

using select_value = void*;

enum select_direction {
  select_send,
  select_recv,
  select_default,
};

namespace bongo {
namespace detail {

// Represents an OS thread.
struct thread {
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic_bool select_done_ = false;

  // Block this forever.
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

struct chan {
  waitq sendq_;
  waitq recvq_;
  size_t const size_;
  std::atomic_size_t count_ = 0;
  size_t sendx_ = 0;
  size_t recvx_ = 0;
  std::atomic_bool closed_ = false;
  std::mutex mutex_;

  chan(size_t size)
      : size_{size} {}

  virtual void reset(select_value value_ptr) noexcept = 0;
  virtual void send(select_value from_ptr, waitq::thread* t) noexcept = 0;
  virtual void send(select_value from_ptr) noexcept = 0;
  virtual void recv(select_value to_ptr, waitq::thread* t) noexcept = 0;
  virtual void recv(select_value to_ptr) noexcept = 0;
};

}  // namespace detail

struct select_case {
  select_direction direction;
  detail::chan* chan = nullptr;
  select_value value = nullptr;
};

template <typename T>
class chan : public detail::chan {
  std::unique_ptr<T[]> buf_ = nullptr;

 public:
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;

  using send_type = value_type;
  using recv_type = std::optional<value_type>;

  chan()
      : chan{0} {}
  explicit chan(size_t n)
      : detail::chan{n} {
    if (size_ > 0) {
      buf_ = std::make_unique<T[]>(size_);
    }
  }

  void send(T const& value) { send(T{value}); }
  void send(T&& value) {
    std::unique_lock chan_lock{mutex_};
    if (closed_.load(std::memory_order_relaxed)) {
      throw logic_error{"send on closed channel"};
    }
    if (auto t = recvq_.dequeue()) {
      // Send to waiting receiver
      send(&value, t);
    } else {
      if (count_.load(std::memory_order_relaxed) < size_) {
        // Send to buffer
        send(&value);
      } else {
        // Block until some receiver completes the operation
        auto t = detail::waitq::thread{};
        t.value_ = &value;
        std::unique_lock lock{t.parent_.mutex_};
        sendq_.enqueue(&t);
        chan_lock.unlock();
        t.parent_.cond_.wait(lock, [&t]() { return t.done_waiting_; });
        if (t.closed_) {
          throw logic_error{"send on closed channel"};
        }
      }
    }
  }

  std::optional<T> recv() {
    std::unique_lock chan_lock{mutex_};
    if (closed_.load(std::memory_order_relaxed) && count_.load(std::memory_order_relaxed) == 0) {
      return std::nullopt;
    }
    if (auto t = sendq_.dequeue()) {
      // Receive from waiting sender
      std::optional<T> value;
      recv(&value, t);
      return value;
    } else {
      if (count_.load(std::memory_order_relaxed) > 0) {
        // Receive from buffer
        std::optional<T> value;
        recv(&value);
        return value;
      } else {
        // Block until some sender completes the operation
        std::optional<T> value;
        auto t = detail::waitq::thread{};
        t.value_ = &value;
        std::unique_lock lock{t.parent_.mutex_};
        recvq_.enqueue(&t);
        chan_lock.unlock();
        t.parent_.cond_.wait(lock, [&t]() { return t.done_waiting_; });
        return value;
      }
    }
  }

  void close() {
    std::unique_lock chan_lock{mutex_};
    if (closed_.load(std::memory_order_relaxed)) {
      throw logic_error{"close of closed channel"};
    }
    closed_.store(true, std::memory_order_relaxed);
    std::vector<detail::waitq::thread*> threads;
    // Release all readers
    while (auto t = recvq_.dequeue()) {
      reset(t->value_);
      threads.push_back(t);
    }
    // Release all writers (they will throw)
    while (auto t = sendq_.dequeue()) {
      threads.push_back(t);
    }
    chan_lock.unlock();
    for (auto t : threads) {
      std::unique_lock lock{t->parent_.mutex_};
      if (t->is_select_) {
        t->parent_.select_done_.store(true, std::memory_order_relaxed);
      }
      t->done_waiting_ = true;
      t->closed_ = true;
      lock.unlock();
      t->parent_.cond_.notify_one();
    }
  }

  size_t cap() const noexcept { return size_; }
  size_t len() const noexcept { return count_.load(std::memory_order_relaxed); }

  select_case send_select_case(T const&& v) = delete;
  select_case send_select_case(T&& v) {
    return select_case{select_send, this, std::addressof(v)};
  }

  select_case recv_select_case(std::optional<T>& v) {
    return select_case{select_recv, this, std::addressof(v)};
  }

  struct iterator {
    chan<T>& chan_;
    std::optional<T> value_;

    using iterator_category = std::input_iterator_tag;
    using value_type = std::optional<T>;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    bool operator==(iterator const& other) { return value_ == other.value_; }
    bool operator!=(iterator const& other) { return value_ != other.value_; }
    reference operator*() { return *value_; }
    pointer operator->() { return &value_; }

    iterator& operator++() {
      value_ = chan_.recv();
      return *this;
    }

    iterator operator++(int) {
      auto it = *this;
      value_ = chan_.recv();
      return it;
    }
  };

  iterator begin() { return iterator{*this, recv()}; }
  iterator end() { return iterator{*this, std::nullopt}; }
  void push_back(T const& value) { send(T{value}); }
  void push_back(T&& value) { send(std::move(value)); }

  void reset(select_value value_ptr) noexcept override {
    auto value = reinterpret_cast<std::optional<T>*>(value_ptr);
    value->reset();
  }

  // Send a value to another thread.
  void send(select_value from_ptr, detail::waitq::thread* t) noexcept override {
    auto from = reinterpret_cast<T*>(from_ptr);
    auto to = reinterpret_cast<std::optional<T>*>(t->value_);
    std::unique_lock lock{t->parent_.mutex_};
    *to = std::move(*from);
    t->done_waiting_ = true;
    lock.unlock();
    t->parent_.cond_.notify_one();
  }

  // Send a value to the buffer.
  void send(select_value from_ptr) noexcept override {
    auto from = reinterpret_cast<T*>(from_ptr);
    buf_[sendx_] = std::move(*from);
    if (++sendx_ == size_) {
      sendx_ = 0;
    }
    count_.fetch_add(1, std::memory_order_relaxed);
  }

  // Receive a value from another thread.
  void recv(select_value to_ptr, detail::waitq::thread* t) noexcept override {
    auto to = reinterpret_cast<std::optional<T>*>(to_ptr);
    auto from = reinterpret_cast<T*>(t->value_);
    std::unique_lock lock{t->parent_.mutex_};
    if (count_.load(std::memory_order_relaxed) == 0) {
      *to = std::move(*from);
    } else {
      // Queue is full
      *to = std::move(buf_[recvx_]);
      buf_[recvx_] = std::move(*from);
      if (++recvx_ == size_) {
        recvx_ = 0;
      }
      sendx_ = recvx_;
    }
    t->done_waiting_ = true;
    lock.unlock();
    t->parent_.cond_.notify_one();
  }

  // Receive a value from the buffer.
  void recv(select_value to_ptr) noexcept override {
    auto to = reinterpret_cast<std::optional<T>*>(to_ptr);
    *to = std::move(buf_[recvx_]);
    if (++recvx_ == size_) {
      recvx_ = 0;
    }
    count_.fetch_sub(1, std::memory_order_relaxed);
  }
};

// Create a send select case.
template <typename T>
select_case send_select_case(chan<T>& c, T&& v) {
  return select_case{select_send, std::addressof(c), std::addressof(v)};
}

template <typename T>
select_case send_select_case(chan<T>* c, T&& v) {
  return select_case{select_send, c, std::addressof(v)};
}

// Create a receive select case.
template <typename T>
select_case recv_select_case(chan<T>& c, std::optional<T>& v) {
  return select_case{select_recv, std::addressof(c), std::addressof(v)};
}

template <typename T>
select_case recv_select_case(chan<T>* c, std::optional<T>& v) {
  return select_case{select_recv, c, std::addressof(v)};
}

// Create a default select case.
inline select_case default_select_case() {
  return select_case{select_default, nullptr, nullptr};
}

// Execute a select operation.
size_t select(select_case const* cases, size_t n);

inline size_t select(std::vector<select_case> const& cases) {
  return select(cases.data(), cases.size());
}

template <size_t N>
size_t select(const select_case (&cases)[N]) {
  return select(cases, N);
}

// Send a value to a channel.
template <typename T>
void operator<<(chan<T>& c, T const& value) {
  c.send(T{value});
}

template <typename T>
void operator<<(chan<T>* c, T const& value) {
  if (c) {
    c->send(T{value});
  } else {
    detail::this_thread().forever_sleep();
  }
}

// Move a value to a channel.
template <typename T>
void operator<<(chan<T>& c, T&& value) {
  c.send(std::move(value));
}

template <typename T>
void operator<<(chan<T>* c, T&& value) {
  if (c) {
    c->send(std::move(value));
  } else {
    detail::this_thread().forever_sleep();
  }
}

// Receive an optional value from a channel.
template <typename T>
void operator<<(std::optional<T>& value, chan<T>& c) {
  value = c.recv();
}

template <typename T>
void operator<<(std::optional<T>& value, chan<T>* c) {
  if (c) {
    value = c->recv();
  } else {
    detail::this_thread().forever_sleep();
  }
}

// Receive a value from a channel.
template <typename T>
void operator<<(T& value, chan<T>& c) {
  auto v = c.recv();
  value = v ? std::move(*v) : T{};
}

template <typename T>
void operator<<(T& value, chan<T>* c) {
  if (c) {
    auto v = c->recv();
    value = v ? std::move(*v) : T{};
  } else {
    detail::this_thread().forever_sleep();
  }
}

}  // namespace bongo
