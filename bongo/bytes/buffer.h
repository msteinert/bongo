// Copyright The Go Authors.

#pragma once

#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/io/error.h>
#include <bongo/io/io.h>

namespace bongo::bytes {

constexpr static long small_buffer_size = 64;

class buffer {
  using allocator = std::allocator<uint8_t>;

  enum class read_op : int8_t {
    read = -1,
    invalid = 0,
    read_rune1 = 1,
    read_rune2 = 2,
    read_rune3 = 3,
    read_rune4 = 4,
  };

  uint8_t* buf_ = nullptr;
  long off_ = 0, size_ = 0, cap_ = 0;
  read_op last_read_ = read_op::invalid;

 public:
  using size_type = long;
  using value_type = uint8_t;
  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = value_type*;
  using const_pointer = value_type const*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  buffer() = default;
  buffer(std::span<uint8_t const> s);
  buffer(std::string_view s);
  buffer(buffer const& other);
  buffer& operator=(buffer const& other);
  buffer(buffer&& other) noexcept;
  buffer& operator=(buffer&& other) noexcept;
  ~buffer();

  auto bytes() noexcept -> std::span<uint8_t>;
  auto bytes() const noexcept -> std::span<uint8_t const>;
  auto str() const -> std::string_view;
  auto size() const noexcept -> long;
  auto capacity() const noexcept -> long;
  auto empty() const noexcept -> bool;
  auto reset() noexcept -> void;
  auto grow(long n) -> long;
  auto truncate(long n) -> void;

  auto write(std::span<uint8_t const> p) -> std::pair<long, std::error_code>;
  auto write_string(std::string_view s) -> std::pair<long, std::error_code>;
  auto write_byte(uint8_t b) noexcept -> std::error_code;
  auto write_rune(rune r) -> std::pair<long, std::error_code>;
  auto read(std::span<uint8_t> p) noexcept -> std::pair<long, std::error_code>;
  auto read_byte() noexcept -> std::pair<uint8_t, std::error_code>;
  auto read_rune() noexcept -> std::tuple<rune, long, std::error_code>;
  auto unread_byte() noexcept -> std::error_code;
  auto unread_rune() noexcept -> std::error_code;
  auto next(long n) noexcept -> std::span<uint8_t>;
  auto read_bytes(uint8_t delim) noexcept -> std::pair<std::span<uint8_t>, std::error_code>;
  auto read_string(uint8_t delim) noexcept -> std::pair<std::string_view, std::error_code>;

  template <typename T> requires io::WriterFunc<T>
  auto write_to(T& w) -> std::pair<int64_t, std::error_code>;

  template <typename T> requires io::Reader<T>
  auto read_from(T& r) -> std::pair<int64_t, std::error_code>;

  auto begin() -> iterator { return buf_ + off_; }
  auto end() -> iterator { return std::next(begin(), size_); }
  auto begin() const -> const_iterator { return buf_ + off_; }
  auto end() const -> const_iterator { return std::next(begin(), size_); }
  auto push_back(uint8_t c) -> void { write_byte(c); }
  auto operator[](long pos) -> reference { return *std::next(begin(), pos); }
  auto operator[](long pos) const -> const_reference { return *std::next(begin(), pos); }
};

template <typename T> requires io::WriterFunc<T>
auto buffer::write_to(T& w) -> std::pair<int64_t, std::error_code> {
  using io::write;
  int64_t n = 0;
  last_read_ = read_op::invalid;
  if (auto n_bytes = size(); n_bytes > 0) {
    auto [m, err] = write(w, bytes());
    if (m > n_bytes) {
      throw std::runtime_error{"bytes::buffer::write_to: invalid write count"};
    }
    off_ += m;
    size_ -= m;
    n = static_cast<int64_t>(m);
    if (err) {
      return {n, err};
    }
    if (m != static_cast<int64_t>(n_bytes)) {
      return {n, io::error::short_write};
    }
  }
  reset();
  return {n, nil};
}

template <typename T> requires io::Reader<T>
auto buffer::read_from(T& r) -> std::pair<int64_t, std::error_code> {
  using io::read;
  constexpr static long min_read = 512;
  int64_t n = 0;
  last_read_ = read_op::invalid;
  for (;;) {
    grow(min_read);
    auto [m, err] = read(r, std::span{end(), static_cast<size_t>(cap_-off_-size_)});
    if (m < 0) {
      throw std::runtime_error{"bytes::buffer: reader returned negative count from read"};
    }
    size_ += m;
    n += static_cast<int64_t>(m);
    if (err == io::eof) {
      return {n, nil};
    }
    if (err) {
      return {n, err};
    }
  }
}

}  // namespace bongo::bytes
