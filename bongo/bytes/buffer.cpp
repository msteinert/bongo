// Copyright The Go Authors.

#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#include "bongo/bytes/buffer.h"
#include "bongo/bytes/bytes.h"
#include "bongo/bytes/error.h"
#include "bongo/io/error.h"
#include "bongo/unicode/utf8.h"

namespace bongo::bytes {
namespace {

static constexpr size_t npos = static_cast<size_t>(-1);

template <typename T>
size_t find_first_of(std::span<T> s, T const& d) {
  size_t i = 0;
  for (; i < s.size(); ++i) {
    if (s[i] == d) {
      return i;
    }
  }
  return npos;
}

}  // namespace

buffer::buffer(std::span<uint8_t> const s) {
  buf_ = allocator{}.allocate(s.size());
  size_ = s.size();
  std::copy(s.begin(), s.end(), buf_);
}

buffer::buffer(std::string_view s) {
  buf_ = allocator{}.allocate(s.size());
  size_ = s.size();
  std::copy(s.begin(), s.end(), buf_);
}

buffer::buffer(buffer const& other) {
  buf_ = allocator{}.allocate(other.cap_);
  off_ = other.off_;
  size_ = other.size_;
  cap_ = other.cap_;
  auto s = other.bytes();
  std::copy(s.begin(), s.end(), buf_+off_);
}

buffer& buffer::operator=(buffer const& other) {
  buf_ = allocator{}.allocate(other.cap_);
  off_ = other.off_;
  size_ = other.size_;
  cap_ = other.cap_;
  auto s = other.bytes();
  std::copy(s.begin(), s.end(), buf_+off_);
  return *this;
}

buffer::buffer(buffer&& other) noexcept {
  std::swap(buf_, other.buf_);
  std::swap(off_, other.off_);
  std::swap(size_, other.size_);
  std::swap(cap_, other.cap_);
  std::swap(last_read_, other.last_read_);
}

buffer& buffer::operator=(buffer&& other) noexcept {
  std::swap(buf_, other.buf_);
  std::swap(off_, other.off_);
  std::swap(size_, other.size_);
  std::swap(cap_, other.cap_);
  std::swap(last_read_, other.last_read_);
  return *this;
}

buffer::~buffer() {
  if (buf_ != nullptr) {
    allocator{}.deallocate(buf_, cap_);
  }
}

std::span<uint8_t> buffer::bytes() noexcept {
  return std::span{buf_+off_, size_};
}

std::span<uint8_t const> buffer::bytes() const noexcept {
  return std::span{buf_+off_, size_};
}

std::string_view buffer::str() const {
  return std::string_view{reinterpret_cast<char const*>(buf_)+off_, size_};
}

size_t buffer::size() const noexcept {
  return size_;
}

size_t buffer::capacity() const noexcept {
  return cap_;
}

bool buffer::empty() const noexcept {
  return size() == 0;
}

void buffer::reset() noexcept {
  off_ = size_ = 0;
  last_read_ = read_op::invalid;
}

int buffer::grow(size_t n) {
  if (n == 0) {
    return 0;
  }
  auto m = size();
  if (m == 0 && off_ != 0) {
    reset();
  }
  if (buf_ == nullptr && n <= small_buffer_size) {
    buf_ = allocator{}.allocate(small_buffer_size);
    cap_ = small_buffer_size;
    return 0;
  }
  if ((cap_-size_)/2 >= n) {
    std::copy(buf_+off_, buf_+off_+size_, buf_);
  } else if (cap_ > std::numeric_limits<size_t>::max()-cap_-n) {
    throw std::invalid_argument{"bytes::buffer: too large"};
  } else {
    size_t cap = 2*cap_ + n;
    auto buf = allocator{}.allocate(cap);
    std::copy(buf_+off_, buf_+off_+size_, buf);
    if (buf_ != nullptr) {
      allocator{}.deallocate(buf_, cap_);
    }
    buf_ = buf;
    cap_ = cap;
  }
  off_ = 0;
  return size_;
}

void buffer::truncate(size_t n) {
  if (n == 0) {
    reset();
    return;
  }
  last_read_ = read_op::invalid;
  if (n > size_) {
    throw std::invalid_argument{"bytes::buffer: truncation out of range"};
  }
  if (off_ > 0) {
    std::copy(buf_+off_, buf_+off_+size_, buf_);
    off_ = 0;
  }
  size_ = n;
}

std::pair<int, std::error_code> buffer::write(std::span<uint8_t const> p) {
  last_read_ = read_op::invalid;
  grow(p.size());
  std::copy(p.begin(), p.end(), buf_+off_+size_);
  size_ += p.size();
  return {p.size(), nil};
}

std::pair<int, std::error_code> buffer::write_string(std::string_view s) {
  last_read_ = read_op::invalid;
  grow(s.size());
  std::copy(s.begin(), s.end(), buf_+off_+size_);
  size_ += s.size();
  return {s.size(), nil};
}

std::error_code buffer::write_byte(uint8_t b) noexcept {
  last_read_ = read_op::invalid;
  auto m = grow(1);
  buf_[m] = b;
  size_ += 1;
  return nil;
}

std::pair<int, std::error_code> buffer::write_rune(rune r) {
  if (static_cast<uint32_t>(r) < unicode::utf8::rune_self) {
    write_byte(static_cast<uint8_t>(r));
    return {1, nil};
  }
  last_read_ = read_op::invalid;
  grow(unicode::utf8::utf_max);
  auto begin = buf_ + off_ + size_;
  auto n = std::distance(begin, unicode::utf8::encode(r, begin));
  size_ += n;
  return {n, nil};
}

std::pair<int, std::error_code> buffer::read(std::span<uint8_t> p) noexcept {
  last_read_ = read_op::invalid;
  if (empty()) {
    reset();
    if (p.size() == 0) {
      return {0, nil};
    }
    return {0, io::eof};
  }
  int n = std::min(size_, p.size());
  if (n > 0) {
    std::copy(buf_+off_, buf_+off_+n, p.begin());
    off_ += n;
    size_ -= n;
    last_read_ = read_op::read;
  }
  return {n, nil};
}

std::pair<uint8_t, std::error_code> buffer::read_byte() noexcept {
  if (empty()) {
    reset();
    return {0, io::eof};
  }
  auto c = buf_[off_++];
  --size_;
  return {c, nil};
}

std::tuple<rune, int, std::error_code> buffer::read_rune() noexcept {
  if (empty()) {
    reset();
    return {0, 0, io::eof};
  }
  auto c = buf_[off_];
  if (c < unicode::utf8::rune_self) {
    ++off_;
    last_read_ = read_op::read_rune1;
    return {static_cast<rune>(c), 1, nil};
  }
  auto [r, n] = unicode::utf8::decode(buf_+off_, buf_+off_+size_);
  off_ += n;
  size_ -= n;
  last_read_ = static_cast<read_op>(n);
  return {r, n, nil};
}

std::error_code buffer::unread_byte() noexcept {
  if (last_read_ == read_op::invalid) {
    return error::unread_byte;
  }
  last_read_ = read_op::invalid;
  if (off_ > 0) {
    --off_;
    ++size_;
  }
  return nil;
}

std::error_code buffer::unread_rune() noexcept {
  if (last_read_ == read_op::invalid) {
    return error::unread_rune;
  }
  if (static_cast<int>(off_) >= static_cast<int>(last_read_)) {
    off_ -= static_cast<int>(last_read_);
    size_ += static_cast<int>(last_read_);
  }
  last_read_ = read_op::invalid;
  return nil;
}

std::span<uint8_t> buffer::next(int n) noexcept {
  last_read_ = read_op::invalid;
  auto m = size();
  if (n > static_cast<int>(m)) {
    n = m;
  }
  auto data = bytes().subspan(0, n);
  off_ += n;
  size_ -= n;
  if (n > 0) {
    last_read_ = read_op::read;
  }
  return data;
}

std::pair<std::span<uint8_t>, std::error_code> buffer::read_bytes(uint8_t delim) noexcept {
  auto s = bytes();
  std::error_code err = nil;
  auto i = find_first_of(s, delim);
  if (i != npos) {
    s = s.subspan(0, i+1);
    off_ += s.size();
    size_ -= s.size();
  } else {
    off_ = cap_;
    size_ = 0;
    err = io::eof;
  }
  last_read_ = read_op::read;
  return {s, err};
}

std::pair<std::string_view, std::error_code> buffer::read_string(uint8_t delim) noexcept {
  auto [bytes, err] = read_bytes(delim);
  if (err) {
    return {to_string(bytes), err};
  }
  return {to_string(bytes), nil};
}

}  // namespace bongo::bytes
