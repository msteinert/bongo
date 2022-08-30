// Copyright The Go Authors.

#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#include "bongo/bongo.h"
#include "bongo/bytes.h"
#include "bongo/strings.h"
#include "bongo/unicode/utf8.h"

namespace bongo::strings {

builder::builder(builder&& other) {
  buf_ = std::exchange(buf_, other.buf_);
  size_ = std::exchange(size_, other.size_);
  cap_ = std::exchange(cap_, other.cap_);
}

builder& builder::operator=(builder&& other) {
  buf_ = std::exchange(buf_, other.buf_);
  size_ = std::exchange(size_, other.size_);
  cap_ = std::exchange(cap_, other.cap_);
  return *this;
}

builder::~builder() {
  if (buf_ != nullptr) {
    allocator{}.deallocate(buf_, cap_);
  }
}

size_t builder::size() const noexcept {
  return size_;
}

size_t builder::capacity() const noexcept {
  return cap_;
}

void builder::reset() noexcept {
  size_ = cap_ = 0;
}

void builder::grow(size_t n) {
  if (n > 0) {
    auto cap = 2*cap_ + n;
    auto buf = allocator{}.allocate(cap);
    if (buf_) {
      std::copy(buf_, buf_+size_, buf);
      allocator{}.deallocate(buf_, cap_);
    }
    buf_ = buf;
    cap_ = cap;
  }
}

std::span<uint8_t> builder::bytes() const noexcept {
  return std::span{reinterpret_cast<uint8_t*>(buf_), size_};
}

std::string_view builder::str() const {
  return std::string_view{buf_, size_};
}

std::pair<long, std::error_code> builder::write(std::span<uint8_t const> p) {
  ensure(p.size());
  std::copy(p.begin(), p.end(), buf_+size_);
  size_ += p.size();
  return {static_cast<long>(p.size()), nil};
}

std::error_code builder::write_byte(uint8_t b) {
  ensure(1);
  buf_[size_++] = b;
  return nil;
}

std::pair<long, std::error_code> builder::write_rune(rune r) {
  if (static_cast<uint32_t>(r) < unicode::utf8::rune_self) {
    write_byte(static_cast<uint8_t>(r));
    return {1, nil};
  }
  ensure(unicode::utf8::utf_max);
  auto begin = buf_ + size_;
  auto n = std::distance(begin, unicode::utf8::encode(r, begin));
  size_ += n;
  return {n, nil};
}

std::pair<long, std::error_code> builder::write_string(std::string_view s) {
  ensure(s.size());
  std::copy(s.begin(), s.end(), buf_+size_);
  size_ += s.size();
  return {s.size(), nil};
}

void builder::ensure(size_t n) {
  auto avail = cap_-size_;
  if (avail < n) {
    grow(n-avail);
  }
}

}  // namespace bongo::strings
