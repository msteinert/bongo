// Copyright The Go Authors.

#pragma once

#include <condition_variable>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <bongo/runtime/detail/chan_impl.h>
#include <bongo/runtime/select.h>

namespace bongo::runtime {

/**
 * The chan type is a bug-for-bug port of Go channels.
 *
 * - https://golang.org/ref/spec#Channel_types
 * - https://tour.golang.org/concurrency/2
 */
template <typename T>
class chan : public detail::chan_impl {
  std::unique_ptr<T[]> buf_ = nullptr;

 public:
  using value_type = T;
  using reference = T&;
  using const_reference = T const&;
  using pointer = T*;
  using const_pointer = T const*;
  using send_type = value_type;
  using recv_type = std::optional<value_type>;

  friend size_t select(select_case const* cases, size_t n);

  chan()
      : chan{0} {}
  explicit chan(size_t n)
      : detail::chan_impl{n} {
    if (size_ > 0) {
      buf_ = std::make_unique<T[]>(size_);
    }
  }

  void send(T const& value) { send(T{value}); }
  void send(T&& value) {
    std::unique_lock chan_lock{mutex_};
    if (closed_.load(std::memory_order_relaxed)) {
      throw std::logic_error{"send on closed channel"};
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
          throw std::logic_error{"send on closed channel"};
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
      throw std::logic_error{"close of closed channel"};
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
      t->parent_.cond_.notify_one();
      lock.unlock();
    }
  }

  size_t cap() const noexcept { return size_; }
  size_t len() const noexcept { return count_.load(std::memory_order_relaxed); }

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

 private:
  void reset(detail::select_value value_ptr) noexcept override {
    auto value = reinterpret_cast<std::optional<T>*>(value_ptr);
    value->reset();
  }

  // Send a value to another thread.
  void send(detail::select_value from_ptr, detail::waitq::thread* t) noexcept override {
    auto from = reinterpret_cast<T*>(from_ptr);
    auto to = reinterpret_cast<std::optional<T>*>(t->value_);
    std::unique_lock lock{t->parent_.mutex_};
    *to = std::move(*from);
    t->done_waiting_ = true;
    t->parent_.cond_.notify_one();
    lock.unlock();
  }

  // Send a value to the buffer.
  void send(detail::select_value from_ptr) noexcept override {
    auto from = reinterpret_cast<T*>(from_ptr);
    buf_[sendx_] = std::move(*from);
    if (++sendx_ == size_) {
      sendx_ = 0;
    }
    count_.fetch_add(1, std::memory_order_relaxed);
  }

  // Receive a value from another thread.
  void recv(detail::select_value to_ptr, detail::waitq::thread* t) noexcept override {
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
    t->parent_.cond_.notify_one();
    lock.unlock();
  }

  // Receive a value from the buffer.
  void recv(detail::select_value to_ptr) noexcept override {
    auto to = reinterpret_cast<std::optional<T>*>(to_ptr);
    *to = std::move(buf_[recvx_]);
    if (++recvx_ == size_) {
      recvx_ = 0;
    }
    count_.fetch_sub(1, std::memory_order_relaxed);
  }
};

/**
 * Send a value to a channel.
 */
template <typename T>
void operator<<(chan<T>& c, T&& value) {
  c.send(std::move(value));
}

/**
 * Send a value to a channel.
 *
 * If \p c is nullptr then this thread will block forever.
 */
template <typename T>
void operator<<(chan<T>* c, T&& value) {
  if (c) {
    c->send(std::move(value));
  } else {
    detail::this_thread().forever_sleep();
  }
}

/**
 * Send a copy of a value to a channel.
 */
template <typename T>
void operator<<(chan<T>& c, T const& value) {
  c.send(T{value});
}

/**
 * Send a copy of a value to a channel.
 *
 * If \p c is nullptr then this thread will block forever.
 */
template <typename T>
void operator<<(chan<T>* c, T const& value) {
  if (c) {
    c->send(T{value});
  } else {
    detail::this_thread().forever_sleep();
  }
}

/**
 * Receive a value from a channel.
 */
template <typename T>
void operator<<(std::optional<T>& value, chan<T>& c) {
  value = c.recv();
}

/**
 * Receive a value from a channel.
 */
template <typename T>
void operator<<(std::optional<T>& value, chan<T>* c) {
  if (c) {
    value = c->recv();
  } else {
    detail::this_thread().forever_sleep();
  }
}

/**
 * Receive a value from a channel.
 *
 * If no value is received a default initialized value is returned.
 */
template <typename T>
void operator<<(T& value, chan<T>& c) {
  auto v = c.recv();
  value = v ? std::move(*v) : T{};
}

/**
 * Receive a value from a channel.
 *
 * If no value is received a default initialized value is returned. If \p c is
 * nullptr then this thread will block forever.
 */
template <typename T>
void operator<<(T& value, chan<T>* c) {
  if (c) {
    auto v = c->recv();
    value = v ? std::move(*v) : T{};
  } else {
    detail::this_thread().forever_sleep();
  }
}

}  // namespace bongo::runtime
