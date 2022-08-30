// Copyright The Go Authors.

#pragma once

#include <span>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/bongo.h>

namespace bongo::strings {

class builder {
  using allocator = std::allocator<char>;

  char* buf_ = nullptr;
  size_t size_ = 0, cap_ = 0;

 public:
  using value_type = char;
  using pointer = value_type*;
  using iterator = pointer;
  using const_iterator = value_type const*;

  builder() = default;
  builder(builder const& other) = delete;
  builder& operator=(builder const& other) = delete;
  builder(builder&& other);
  builder& operator=(builder&& other);
  ~builder();

  size_t size() const noexcept;
  size_t capacity() const noexcept;
  void reset() noexcept;
  void grow(size_t n);
  std::span<uint8_t> bytes() const noexcept;
  std::string_view str() const;

  std::pair<long, std::error_code> write(std::span<uint8_t const> p);
  std::error_code write_byte(uint8_t b);
  std::pair<long, std::error_code> write_rune(rune r);
  std::pair<long, std::error_code> write_string(std::string_view s);

  iterator begin() { return buf_; }
  iterator end() { return buf_ + size_; }
  const_iterator begin() const { return buf_; }
  const_iterator end() const { return buf_ + size_; }
  void push_back(char c) { write_byte(c); }

 private:
  void ensure(size_t n);
};

}  // namespace bongo::strings
