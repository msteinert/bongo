// Copyright The Go Authors.

#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/bufio/error.h>
#include <bongo/io.h>

namespace bongo::bufio {

template <typename T> requires io::WriterFunc<T>
class writer {
  std::vector<uint8_t> buf_;
  T* wr_;
  long n_;
  std::error_code err_;

  static constexpr long max_consecutive_empty_reads = 100;

 public:
  writer() = delete;
  writer(T& wr, long size) { reset(wr, size); }
  writer(T& wr) { reset(wr); }
  writer(writer const& other) = delete;
  writer(writer&& other) = default;
  writer& operator=(writer const& other) = delete;
  writer& operator=(writer&& other) = default;
  ~writer();

  auto size() -> long { return buf_.size(); }
  auto buffered() -> long { return n_; }
  auto reset(T& wr) -> void { reset(wr, 4096); }
  auto reset(T& wr, long size) -> void;
  auto flush() -> std::error_code;
  auto available() -> long { return buf_.size() - n_; }
  auto available_buffer() -> std::span<uint8_t> { return std::span{std::next(buf_.begin(), n_), buf_.size()-n_}; }
  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code>;
  auto write_byte(uint8_t c) -> std::error_code;
  auto write_rune(rune c) -> std::pair<long, std::error_code>;
  auto write_string(std::string_view s) -> std::pair<long, std::error_code>;

  template <typename U> requires io::ReaderFunc<U>
  auto read_from(U& r) -> std::pair<int64_t, std::error_code>;
};

template <typename T>
writer<T>::~writer() {
  flush();
}

template <typename T>
auto writer<T>::reset(T& wr, long size) -> void {
  if (size < 0) {
    size = 4096;
  }
  buf_.clear();
  buf_.resize(size);
  n_ = 0;
  wr_ = std::addressof(wr);
  err_ = nil;
}

template <typename T>
auto writer<T>::flush() -> std::error_code {
  using io::write;
  if (err_ != nil) {
    return err_;
  }
  if (n_ == 0) {
    return nil;
  }
  auto [n, err] = write(*wr_, std::span{buf_.begin(), static_cast<size_t>(n_)});
  if (n < n_ && err == nil) {
    err = io::error::short_write;
  }
  if (err != nil) {
    if (n > 0 && n < n_) {
      std::copy(std::next(buf_.begin(), n), std::next(buf_.begin(), n_-n), buf_.begin());
    }
    n_ -= n;
    err_ = err;
    return err;
  }
  n_ = 0;
  return nil;
}

template <typename T>
auto writer<T>::write(std::span<uint8_t const> p) -> std::pair<long, std::error_code> {
  using io::write;
  long nn = 0;
  while (p.size() > static_cast<decltype(p)::size_type>(available()) && err_ == nil) {
    long n = 0;
    if (buffered() == 0) {
      std::tie(n, err_) = write(*wr_, p);
    } else {
      n = std::min(buf_.size() - n_, p.size());
      std::copy(p.begin(), std::next(p.begin(), n), std::next(buf_.begin(), n_));
      n_ += n;
      flush();
    }
    nn += n;
    p = p.subspan(n);
  }
  if (err_ != nil) {
    return {nn, err_};
  }
  auto n = std::min(buf_.size() - n_, p.size());
  std::copy(p.begin(), std::next(p.begin(), n), std::next(buf_.begin(), n_));
  n_ += n;
  nn += n;
  return {nn, nil};
}

template <typename T>
auto writer<T>::write_byte(uint8_t c) -> std::error_code {
  if (err_ != nil) {
    return err_;
  }
  if (available() <= 0 && flush() != nil) {
    return err_;
  }
  buf_[n_++] = c;
  return nil;
}

template <typename T>
auto writer<T>::write_rune(rune r) -> std::pair<long, std::error_code> {
  namespace utf8 = unicode::utf8;
  if (static_cast<uint32_t>(r) < utf8::rune_self) {
    auto err = write_byte(static_cast<uint8_t>(r));
    if (err != nil) {
      return {0, err};
    }
    return {1, nil};
  }
  if (err_ != nil) {
    return {0, err_};
  }
  auto n = available();
  if (n < utf8::utf_max) {
    if (flush(); err_ != nil) {
      return {0, err_};
    }
    n = available();
    if (n < utf8::utf_max) {
      return write_string(utf8::encode(r));
    }
  }
  auto out = std::next(buf_.begin(), n_);
  long size = std::distance(out, utf8::encode(r, out));
  n_ += size;
  return {size, nil};
}

template <typename T>
auto writer<T>::write_string(std::string_view s) -> std::pair<long, std::error_code> {
  long nn = 0;
  while (s.size() > static_cast<size_t>(available()) && err_ == nil) {
    long n = 0;
    if (buffered() == 0) {
      if constexpr (io::StringWriterFunc<T>) {
        using io::write_string;
        std::tie(n, err_) = write_string(*wr_, s);
      }
    } else {
      n = std::min(buf_.size() - n_, s.size());
      std::copy(s.begin(), std::next(s.begin(), n), std::next(buf_.begin(), n_));
      n_ += n;
      flush();
    }
    nn += n;
    s = s.substr(n);
  }
  if (err_ != nil) {
    return {nn, err_};
  }
  auto n = std::min(buf_.size() - n_, s.size());
  std::copy(s.begin(), std::next(s.begin(), n), std::next(buf_.begin(), n_));
  n_ += n;
  nn += n;
  return {nn, nil};
}

template <typename T>
template <typename U> requires io::ReaderFunc<U>
auto writer<T>::read_from(U& r) -> std::pair<int64_t, std::error_code> {
  using io::read;
  if (err_ != nil) {
    return {0, err_};
  }
  int64_t n = 0;
  long m = 0;
  std::error_code err;
  for (;;) {
    if (available() == 0) {
      if (auto err1 = flush(); err1 != nil) {
        return {0, err1};
      }
    }
    if constexpr (io::ReaderFromFunc<T, U>) {
      if (buffered() == 0) {
        using io::read_from;
        auto [nn, err] = read_from(*wr_, r);
        err_ = err;
        n += nn;
        return {n, err};
      }
    }
    long nr = 0;
    while (nr < max_consecutive_empty_reads) {
      std::tie(m, err) = read(r, std::span{std::next(buf_.begin(), n_), static_cast<size_t>(buf_.size()-n_)});
      if (m != 0 || err != nil) {
        break;
      }
      ++nr;
    }
    if (nr == max_consecutive_empty_reads) {
      return {n, io::error::no_progress};
    }
    n_ += m;
    n += static_cast<int64_t>(m);
    if (err != nil) {
      break;
    }
  }
  if (err == io::eof) {
    if (available() == 0) {
      err = flush();
    } else {
      err = nil;
    }
  }
  return {n, err};
}

}  // namespace bongo::bufio
