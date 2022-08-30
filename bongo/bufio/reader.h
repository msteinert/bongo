// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <memory>
#include <span>
#include <system_error>
#include <tuple>
#include <utility>

#include <bongo/bufio/error.h>
#include <bongo/bytes/bytes.h>
#include <bongo/io/io.h>
#include <bongo/strings/builder.h>
#include <bongo/unicode/utf8.h>

namespace bongo::bufio {

template <typename T> requires io::ReaderFunc<T>
class reader {
  std::vector<uint8_t> buf_;
  T* rd_;
  long r_, w_;
  std::error_code err_;
  long last_byte_, last_rune_size_;

  static constexpr long min_read_buffer_size = 16;
  static constexpr long max_consecutive_empty_reads = 100;

 public:
  reader() = delete;
  reader(T& rd, long size) { reset(rd, size); }
  reader(T& rd) { reset(rd); }
  ~reader() = default;
  reader(reader const& other) = delete;
  reader(reader&& other) = default;
  reader& operator=(reader const& other) = delete;
  reader& operator=(reader&& other) = default;

  auto size() -> long { return buf_.size(); }
  auto buffered() -> long { return w_ - r_; }
  auto reset(T& rd) -> void { reset(rd, 4096); }
  auto reset(T& rd, long size) -> void;
  auto peek(long n) -> std::pair<std::span<uint8_t>, std::error_code>;
  auto discard(long n) -> std::pair<long, std::error_code>;
  auto read(std::span<uint8_t> p) -> std::pair<long, std::error_code>;
  auto read_byte() -> std::pair<uint8_t, std::error_code>;
  auto unread_byte() -> std::error_code;
  auto read_rune() -> std::tuple<rune, long, std::error_code>;
  auto unread_rune() -> std::error_code;
  auto read_slice(uint8_t delim) -> std::pair<std::span<uint8_t>, std::error_code>;
  auto read_line() -> std::tuple<std::span<uint8_t>, bool, std::error_code>;
  auto read_bytes(uint8_t delim) -> std::pair<std::vector<uint8_t>, std::error_code>;
  auto read_string(uint8_t delim) -> std::pair<std::string, std::error_code>;

  template <typename U> requires io::WriterFunc<U> || io::WriterToFunc<T, U> || io::ReaderFromFunc<U, T>
  auto write_to(U& wr) -> std::pair<int64_t, std::error_code>;

 private:
  auto to_span() -> std::span<uint8_t>;
  auto read_err() -> std::error_code;
  auto fill() -> void;
  auto collect_fragments(uint8_t delim) -> std::tuple<std::vector<std::vector<uint8_t>>, std::span<uint8_t>, long, std::error_code>;

  template <typename U> requires io::WriterFunc<U>
  auto write_buf(U& wr) -> std::pair<int64_t, std::error_code>;
};

template <typename T>
auto reader<T>::reset(T& rd, long size) -> void {
  if (size < min_read_buffer_size) {
    size = min_read_buffer_size;
  }
  buf_.clear();
  buf_.resize(size);
  rd_ = std::addressof(rd);
  r_ = w_ = 0;
  err_ = nil;
  last_byte_ = -1;
  last_rune_size_ = -1;
}

template <typename T>
auto reader<T>::peek(long n) -> std::pair<std::span<uint8_t>, std::error_code> {
  if (n < 0) {
    return {nil, error::negative_count};
  }

  last_byte_ = -1;
  last_rune_size_ = -1;

  while (w_-r_ < n && static_cast<size_t>(w_-r_) < buf_.size() && err_ == nil) {
    fill();
  }

  if (static_cast<size_t>(n) > buf_.size()) {
    return {to_span(), error::buffer_full};
  }

  std::error_code err;
  if (auto avail = w_ - r_; avail < n) {
    n = avail;
    err = read_err();
    if (err == nil) {
      err = error::buffer_full;
    }
  }

  return {std::span{std::next(buf_.begin(), r_), static_cast<size_t>(n)}, err};
}

template <typename T>
auto reader<T>::discard(long n) -> std::pair<long, std::error_code> {
  if (n < 0) {
    return {0, error::negative_count};
  }
  if (n == 0) {
    return {0, nil};
  }

  last_byte_ = -1;
  last_rune_size_ = -1;

  auto remain = n;
  for (;;) {
    auto skip = buffered();
    if (skip == 0) {
      fill();
      skip = buffered();
    }
    if (skip > remain) {
      skip = remain;
    }
    r_ += skip;
    remain -= skip;
    if (remain == 0) {
      return {n, nil};
    }
    if (err_ != nil) {
      return {n - remain, read_err()};
    }
  }
}

template <typename T>
auto reader<T>::read(std::span<uint8_t> p) -> std::pair<long, std::error_code> {
  using io::read;
  long n = p.size();
  if (n == 0) {
    if (buffered() > 0) {
      return {0, nil};
    }
    return {0, read_err()};
  }
  if (r_ == w_) {
    if (err_ != nil) {
      return {0, read_err()};
    }
    if (p.size() >= buf_.size()) {
      std::tie(n, err_) = read(*rd_, p);
      if (n < 0) {
        throw std::system_error{error::negative_read};
      }
      if (n > 0) {
        last_byte_ = static_cast<long>(p[n-1]);
        last_rune_size_ = -1;
      }
      return {n, read_err()};
    }
    r_ = w_ = 0;
    std::tie(n, err_) = read(*rd_, buf_);
    if (n < 0) {
      throw std::system_error{error::negative_read};
    }
    if (n == 0) {
      return {0, read_err()};
    }
    w_ += n;
  }
  n = std::min(w_-r_, static_cast<long>(p.size()));
  auto begin = std::next(buf_.begin(), r_);
  std::copy(begin, std::next(begin, n), p.begin());
  r_ += n;
  last_byte_ = static_cast<long>(buf_[r_-1]);
  last_rune_size_ = -1;
  return {n, nil};
}

template <typename T>
auto reader<T>::read_byte() -> std::pair<uint8_t, std::error_code> {
  last_rune_size_ = -1;
  while (r_ == w_) {
    if (err_ != nil) {
      return {0, read_err()};
    }
    fill();
  }
  auto c = buf_[r_];
  ++r_;
  last_byte_ = static_cast<long>(c);
  return {c, nil};
}

template <typename T>
auto reader<T>::unread_byte() -> std::error_code {
  if (last_byte_ < 0 || r_ == 0 && w_ > 0) {
    return error::invalid_unread_byte;
  }
  if (r_ > 0) {
    --r_;
  } else {
    w_ = 1;
  }
  buf_[r_] = static_cast<uint8_t>(last_byte_);
  last_byte_ = -1;
  last_rune_size_ = -1;
  return nil;
}

template <typename T>
auto reader<T>::read_rune() -> std::tuple<rune, long, std::error_code> {
  namespace utf8 = unicode::utf8;
  while (r_+utf8::utf_max > w_ && !utf8::full_rune(to_span()) && err_ == nil && w_-r_ < size()) {
    fill();
  }
  last_rune_size_ = -1;
  if (r_ == w_) {
    return {0, 0, read_err()};
  }
  long size = 1;
  auto r = static_cast<rune>(buf_[r_]);
  if (r >= utf8::rune_self) {
    std::tie(r, size) = utf8::decode(to_span());
  }
  r_ += size;
  last_byte_ = static_cast<long>(buf_[r_-1]);
  last_rune_size_ = size;
  return {r, size, nil};
}

template <typename T>
auto reader<T>::unread_rune() -> std::error_code {
  if (last_rune_size_ < 0 || r_ < last_rune_size_) {
    return error::invalid_unread_rune;
  }
  r_ -= last_rune_size_;
  last_byte_ = -1;
  last_rune_size_ = -1;
  return nil;
}

template <typename T>
auto reader<T>::read_slice(uint8_t delim) -> std::pair<std::span<uint8_t>, std::error_code> {
  std::span<uint8_t> line;
  std::error_code err;
  auto s = 0;
  for (;;) {
    if (auto i = bytes::index(to_span().subspan(s), delim); i != bytes::npos) {
      i += s;
      line = std::span{std::next(buf_.begin(), r_), i+1};
      r_ += i + 1;
      break;
    }

    if (err_ != nil) {
      line = to_span();
      r_ = w_;
      err = read_err();
      break;
    }

    if (buffered() >= size()) {
      r_ = w_;
      line = std::span{buf_};
      err = error::buffer_full;
      break;
    }

    s = w_ - r_;
    fill();
  }

  if (auto i = line.size(); i > 0) {
    last_byte_ = static_cast<long>(line[i-1]);
    last_rune_size_ = -1;
  }

  return {line, err};
}

template <typename T>
auto reader<T>::read_line() -> std::tuple<std::span<uint8_t>, bool, std::error_code> {
  auto [line, err] = read_slice('\n');
  if (err == error::buffer_full) {
    if (line.size() > 0 && line[line.size()-1] == '\r') {
      if (r_ == 0) {
        // unreachable
        throw std::system_error{error::rewind};
      }
      --r_;
      line = line.subspan(0, line.size()-1);
    }
    return {line, true, nil};
  }

  if (line.size() == 0) {
    if (err != nil) {
      line = nil;
    }
    return {line, false, err};
  }
  err = nil;

  if (line.back() == '\n') {
    auto drop = 1;
    if (line.size() > 1 && line[line.size()-2] == '\r') {
      drop = 2;
    }
    line = line.subspan(0, line.size()-drop);
  }
  return {line, false, err};
}

template <typename T>
auto reader<T>::read_bytes(uint8_t delim) -> std::pair<std::vector<uint8_t>, std::error_code> {
  auto [full, frag, n, err] = collect_fragments(delim);
  auto buf = std::vector<uint8_t>{};
  buf.resize(n);
  n = 0;
  for (size_t i = 0; i < full.size(); ++i) {
    std::copy(full[i].begin(), full[i].end(), std::next(buf.begin(), n));
    n += full[i].size();
  }
  std::copy(frag.begin(), frag.end(), std::next(buf.begin(), n));
  return {buf, err};
}

template <typename T>
auto reader<T>::read_string(uint8_t delim) -> std::pair<std::string, std::error_code> {
  auto [full, frag, n, err] = collect_fragments(delim);
  auto buf = strings::builder{};
  buf.grow(n);
  for (auto const& fb : full) {
    buf.write(fb);
  }
  buf.write(frag);
  return {std::string{buf.str()}, err};
}

template <typename T>
template <typename U> requires io::WriterFunc<U> || io::WriterToFunc<T, U> || io::ReaderFromFunc<U, T>
auto reader<T>::write_to(U& wr) -> std::pair<int64_t, std::error_code> {
  last_byte_ = -1;
  last_rune_size_ = -1;

  auto [n, err] = write_buf(wr);
  if (err != nil) {
    return {n, err};
  }

  if constexpr (io::WriterToFunc<T, U>) {
    using io::write_to;
    auto [m, err] = write_to(*rd_, wr);
    n += m;
    return {n, err};
  }

  if constexpr (io::ReaderFromFunc<U, T>) {
    using io::read_from;
    auto [m, err] = read_from(wr, *rd_);
    n += m;
    return {n, err};
  }

  if (w_-r_ < size()) {
    fill();
  }

  while (r_ < w_) {
    auto [m, err] = write_buf(wr);
    n += m;
    if (err != nil) {
      return {n, err};
    }
    fill();
  }

  if (err_ == io::eof) {
    err_ = nil;
  }

  return {n, read_err()};
}

template <typename T>
auto reader<T>::to_span() -> std::span<uint8_t> {
  return std::span{std::next(buf_.begin(), r_), static_cast<size_t>(w_-r_)};
}

template <typename T>
auto reader<T>::read_err() -> std::error_code {
  auto err = err_;
  err_ = nil;
  return err;
}

template <typename T>
auto reader<T>::fill() -> void {
  // Shift data to the beginning of the buffer.
  if (r_ > 0) {
    std::copy(std::next(buf_.begin(), r_), std::next(buf_.begin(), w_), buf_.begin());
    w_ -= r_;
    r_ = 0;
  }

  if (w_ >= size()) {
    throw std::system_error{error::full_buffer};
  }

  // Try to read data a limited number of times.
  for (auto i = max_consecutive_empty_reads; i > 0; --i) {
    using io::read;
    auto [n, err] = read(*rd_, std::span{std::next(buf_.begin(), w_), buf_.size()-w_});
    if (n < 0) {
      throw std::system_error{error::negative_read};
    }
    w_ += n;
    if (err != nil) {
      err_ = err;
      return;
    }
    if (n > 0) {
      return;
    }
  }
  err_ = io::error::no_progress;
}

template <typename T>
auto reader<T>::collect_fragments(uint8_t delim) -> std::tuple<std::vector<std::vector<uint8_t>>, std::span<uint8_t>, long, std::error_code> {
  std::vector<std::vector<uint8_t>> full_buffers;
  std::span<uint8_t> frag;
  long total_len = 0;
  std::error_code err;
  for (;;) {
    std::error_code e;
    std::tie(frag, e) = read_slice(delim);
    if (e == nil) {
      break;
    }
    if (e != error::buffer_full) {
      err = e;
      break;
    }

    full_buffers.push_back({});
    full_buffers.back().assign(frag.begin(), frag.end());
    total_len += static_cast<long>(full_buffers.back().size());
  }
  total_len += static_cast<long>(frag.size());
  return {full_buffers, frag, total_len, err};
}

template <typename T>
template <typename U> requires io::WriterFunc<U>
auto reader<T>::write_buf(U& wr) -> std::pair<int64_t, std::error_code> {
  using io::write;
  auto [n, err] = write(wr, to_span());
  if (n < 0) {
    throw std::system_error{error::negative_write};
  }
  r_ += n;
  return {static_cast<int64_t>(n), err};
}

}  // namespace bongo::bufio
