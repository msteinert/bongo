// Copyright The Go Authors.

#include <algorithm>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>

#include "bongo/bongo.h"
#include "bongo/io.h"
#include "bongo/strings/error.h"
#include "bongo/strings/reader.h"
#include "bongo/unicode/utf8.h"

namespace bongo::strings {

size_t reader::size() const noexcept {
  if (i_ >= s_.size()) {
    return 0;
  }
  return s_.size() - i_;
}

size_t reader::total_size() const noexcept {
  return s_.size();
}

std::pair<long, std::error_code> reader::read(std::span<uint8_t> p) {
  if (i_ >= s_.size()) {
    return {0, io::eof};
  }
  prev_rune_ = -1;
  auto s = std::string_view{std::next(s_.begin(), i_), s_.end()};
  auto n = std::min(s.size(), p.size());
  std::copy_n(s.begin(), n, p.begin());
  i_ += n;
  return {n, nil};
}

std::pair<long, std::error_code> reader::read_at(std::span<uint8_t> p, int64_t off) {
  if (off < 0) {
    return {0, io::error::offset};
  }
  if (static_cast<size_t>(off) >= s_.size()) {
    return {0, io::eof};
  }
  auto s = std::string_view{std::next(s_.begin(), off), s_.end()};
  auto n = std::min(s.size(), p.size());
  std::copy_n(s.begin(), n, p.begin());
  if (n < p.size()) {
    return {n, io::eof};
  }
  return {n, nil};
}

std::pair<uint8_t, std::error_code> reader::read_byte() {
  prev_rune_ = -1;
  if (i_ >= s_.size()) {
    return {0, io::eof};
  }
  return {s_[i_++], nil};
}

std::error_code reader::unread_byte() {
  if (i_ <= 0) {
    return error::at_beginning;
  }
  prev_rune_ = -1;
  --i_;
  return nil;
}

std::tuple<rune, long, std::error_code> reader::read_rune() {
  namespace utf8 = unicode::utf8;
  if (i_ >= s_.size()) {
    prev_rune_ = -1;
    return {0, 0, io::eof};
  }
  prev_rune_ = static_cast<long>(i_);
  if (auto c = s_[i_]; utf8::to_rune(c) < unicode::utf8::rune_self) {
    ++i_;
    return {utf8::to_rune(c), 1, nil};
  }
  auto [ch, size] = unicode::utf8::decode(std::next(s_.begin(), i_), s_.end());
  i_ += size;
  return {ch, size, nil};
}

std::error_code reader::unread_rune() {
  if (i_ <= 0) {
    return error::at_beginning;
  }
  if (prev_rune_ < 0) {
    return error::prev_read_rune;
  }
  i_ = static_cast<size_t>(prev_rune_);
  prev_rune_ = -1;
  return nil;
}

std::pair<int64_t, std::error_code> reader::seek(int64_t offset, long whence) {
  prev_rune_ = -1;
  int64_t abs;
  switch (whence) {
  case io::seek_start:
    abs = offset;
    break;
  case io::seek_current:
    abs = i_ + offset;
    break;
  case io::seek_end:
    abs = s_.size() + offset;
    break;
  default:
    return {0, io::error::whence};
  }
  if (abs < 0) {
    return {0, error::negative_position};
  }
  i_ = abs;
  return {abs, nil};
}

void reader::reset(std::string_view s) {
  s_ = s;
  i_ = 0;
  prev_rune_ = -1;
}

}  // namespace bongo::strings
