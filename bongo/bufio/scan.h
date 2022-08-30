// Copyright The Go Authors.

#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <bongo/bongo.h>
#include <bongo/bufio/error.h>
#include <bongo/bytes/bytes.h>
#include <bongo/io/io.h>
#include <bongo/unicode/utf8.h>

namespace bongo::bufio {

template <typename T>
concept SplitFunction = requires (T func, std::span<uint8_t const> data, bool at_eof) {
  { func(data, at_eof) } -> std::same_as<std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code>>;
};

auto scan_bytes(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code>;
auto scan_runes(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code>;
auto scan_lines(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code>;
auto scan_words(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code>;

static constexpr long max_scan_token_size = 64 * 1024;

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
class scanner {
  T* r_;
  U split_;
  long max_token_size_ = max_scan_token_size;
  std::span<uint8_t const> token_;
  std::vector<uint8_t> buf_;
  long start_ = 0, end_ = 0;
  std::error_code err_;
  long empties_ = 0;
  bool scan_called_ = false;
  bool done_ = false;

  static constexpr long start_buf_size = 4096;
  static constexpr long max_consecutive_empty_reads = 100;

 public:
  scanner() = delete;
  scanner(T& r, U const& split)
      : r_{std::addressof(r)}
      , split_{split} {}
  scanner(T& r, U&& split)
      : r_{std::addressof(r)}
      , split_{std::move(split)} {}

  auto err() -> std::error_code;
  auto scan() -> bool;
  auto bytes() -> std::span<uint8_t const>;
  auto text() -> std::string_view;
  auto buffer(std::vector<uint8_t>&& buf, long max) -> void;

  // Testing
  auto max_token_size(long n) -> void;

 private:
  auto advance(long n) -> bool;
  auto set_err(std::error_code err) -> void;
  auto to_span() -> std::span<uint8_t>;
};

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::err() -> std::error_code {
  if (err_ == io::eof) {
    return nil;
  }
  return err_;
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::scan() -> bool {
  if (done_) {
    return false;
  }
  scan_called_ = true;
  for (;;) {
    if (end_ > start_ || err_ != nil) {
      auto [adv, tok, err] = split_(to_span(), err_ != nil);
      if (err != nil) {
        if (err == error::final_token) {
          token_ = tok.value_or(std::span<uint8_t const>{});
          done_ = true;
          return true;
        }
        set_err(err);
        return false;
      }
      if (!advance(adv)) {
        return false;
      }
      if (tok.has_value()) {
        token_ = tok.value();
        if (err_ == nil || adv > 0) {
          empties_ = 0;
        } else {
          ++empties_;
          if (empties_ > max_consecutive_empty_reads) {
            throw std::system_error{error::too_many_empty_tokens};
          }
        }
        return true;
      }
    }
    if (err_ != nil) {
      start_ = end_ = 0;
      return false;
    }
    if (start_ > 0 && (end_ == static_cast<long>(buf_.size()) || start_ > static_cast<long>(buf_.size()/2))) {
      std::copy(std::next(buf_.begin(), start_), std::next(buf_.begin(), end_), buf_.begin());
      end_ -= start_;
      start_ = 0;
    }
    if (end_ == static_cast<long>(buf_.size())) {
      if (static_cast<long>(buf_.size()) >= max_token_size_ || buf_.size() > std::numeric_limits<long>::max()/2) {
        set_err(error::token_too_long);
        return false;
      }
      long new_size = buf_.size() * 2;
      if (new_size == 0) {
        new_size = start_buf_size;
      }
      if (new_size > max_token_size_) {
        new_size = max_token_size_;
      }
      buf_.resize(new_size);
      std::copy(buf_.begin()+start_, buf_.begin()+end_, buf_.begin());
      end_ -= start_;
      start_ = 0;
    }
    for (long loop = 0;;) {
      auto [n, err] = io::read(*r_, std::span{std::next(buf_.begin(), end_), buf_.size()-end_});
      if (n < 0 || buf_.size()-end_ < static_cast<size_t>(n)) {
        set_err(error::bad_read_count);
        break;
      }
      end_ += n;
      if (err != nil) {
        set_err(err);
        break;
      }
      if (n > 0) {
        empties_ = 0;
        break;
      }
      if (++loop > max_consecutive_empty_reads) {
        set_err(io::error::no_progress);
        break;
      }
    }
  }
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::bytes() -> std::span<uint8_t const> {
  return token_;
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::text() -> std::string_view {
  return bytes::to_string(token_);
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::buffer(std::vector<uint8_t>&& buf, long max) -> void {
  if (scan_called_) {
    throw std::system_error{error::buffer_after_scan};
  }
  buf_ = std::move(buf);
  buf_.resize(buf_.capacity());
  max_token_size_ = max;
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::advance(long n) -> bool {
  if (n < 0) {
    set_err(error::negative_advance);
    return false;
  }
  if (n > end_-start_) {
    set_err(error::advance_too_far);
    return false;
  }
  start_ += n;
  return true;
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::max_token_size(long n) -> void {
  namespace utf8 = unicode::utf8;
  if (n < utf8::utf_max || n > 1e9) {
    throw std::runtime_error{"bad max token size"};
  }
  if (static_cast<size_t>(n) < buf_.size()) {
    buf_.resize(n);
  }
  max_token_size_ = n;
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::set_err(std::error_code err) -> void {
  if (err_ == nil || err_ == io::eof) {
    err_ = err;
  }
}

template <typename T, typename U> requires io::Reader<T> && SplitFunction<U>
auto scanner<T, U>::to_span() -> std::span<uint8_t> {
  return std::span{std::next(buf_.begin(), start_), static_cast<size_t>(end_-start_)};
}

}  // namespace bongo::bufio
