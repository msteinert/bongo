// Copyright The Go Authors.

#pragma once

#include <span>
#include <span>
#include <system_error>
#include <tuple>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/io.h>

namespace bongo::bytes {

class reader {
  std::span<uint8_t const> s_;
  size_t i_ = 0;
  int prev_rune_ = -1;

 public:
  reader() = default;
  reader(std::span<uint8_t const> s)
      : s_{s} {}

  size_t size() const noexcept;
  size_t total_size() const noexcept;
  std::pair<int, std::error_code> read(std::span<uint8_t> p);
  std::pair<int, std::error_code> read_at(std::span<uint8_t> p, int64_t off);
  std::pair<uint8_t, std::error_code> read_byte();
  std::error_code unread_byte();
  std::tuple<rune, int, std::error_code> read_rune();
  std::error_code unread_rune();
  std::pair<int64_t, std::error_code> seek(int64_t offset, int whence);
  void reset(std::span<uint8_t const> s);

  template <typename Writer>
  std::pair<int64_t, std::error_code> write_to(Writer& t);
};

template <typename Writer>
std::pair<int64_t, std::error_code> reader::write_to(Writer& w) {
  prev_rune_ = -1;
  if (i_ >= s_.size()) {
    return {0, nil};
  }
  auto s = std::span{std::next(s_.begin(), i_), s_.end()};
  auto [m, err] = io::write(w, s);
  if (static_cast<size_t>(m) > s_.size()) {
    throw std::runtime_error{"bytes::reader::write_to: invalid write_string count"};
  }
  i_ += m;
  if (static_cast<size_t>(m) != s_.size() && err == nil) {
    err = io::error::short_write;
  }
  return {m, err};
}

}  // namesapce bongo::bytes
