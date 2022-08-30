// Copyright The Go Authors.

#pragma once

#include <memory>
#include <span>
#include <system_error>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/io/io.h>
#include <bongo/testing/iotest/error.h>

namespace bongo::testing::iotest {

template <typename T> requires io::ReaderFunc<T>
class one_byte_reader {
  T* r_;

 public:
  one_byte_reader(T& r)
      : r_{std::addressof(r)} {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    using io::read;
    if (p.size() == 0) {
      return {0, nil};
    }
    return read(*r_, p.subspan(0, 1));
  }
};

template <typename T> requires io::ReaderFunc<T>
class half_reader {
  T* r_;

 public:
  half_reader(T& r)
      : r_{std::addressof(r)} {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    using io::read;
    return read(*r_, p.subspan(0, (p.size()+1)/2));
  }
};

template <typename T> requires io::ReaderFunc<T>
class data_error_reader {
  T* r_;
  std::vector<uint8_t> data_;
  std::span<uint8_t> unread_;

 public:
  data_error_reader(T& r)
      : r_{std::addressof(r)}
      , data_(1024, 0) {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    using io::read;
    long n = 0;
    std::error_code err;
    for (;;) {
      if (unread_.size() == 0) {
        auto [n1, err1] = read(*r_, data_);
        unread_ = std::span{data_.begin(), static_cast<size_t>(n1)};
        err = err1;
      }
      if (n > 0 || err != nil) {
        break;
      }
      n = std::min(p.size(), unread_.size());
      std::copy(unread_.begin(), std::next(unread_.begin(), n), p.begin());
      unread_ = unread_.subspan(n);
    }
    return {n, err};
  }
};

template <typename T> requires io::ReaderFunc<T>
class timeout_reader {
  T* r_;
  long count_ = 0;

 public:
  timeout_reader(T& r)
      : r_{std::addressof(r)} {}

  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
    using io::read;
    ++count_;
    if (count_ == 2) {
      return {0, error::timeout};
    }
    return read(*r_, p);
  }
};

}  // namespace bongo::testing::iotest
