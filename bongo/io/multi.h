// Copyright The Go Authors.

#pragma once

#include <span>
#include <system_error>
#include <tuple>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/bytes.h>
#include <bongo/io/detail/visit_at.h>
#include <bongo/io/io.h>

namespace bongo::io {

template <Reader... Ts>
class multi_reader {
  std::tuple<Ts&...> readers_;
  size_t size_;
  size_t i_ = 0;

 public:
  multi_reader(Ts&... readers)
      : readers_{readers...}
      , size_{sizeof... (readers)} {}

  std::pair<int, std::error_code> read(std::span<uint8_t> p) {
    using io::read;
    while (i_ < size_) {
      auto [n, err] = detail::visit_at([&](auto& r) {
        return read(r, p);
      }, readers_, i_);
      if (err == eof) {
        ++i_;
      }
      if (n > 0 || err != eof) {
        if (err == eof && i_ < size_) {
          err = nil;
        }
        return {n, err};
      }
    }
    return {0, eof};
  }
};

template <Writer... Ts>
class multi_writer {
  std::tuple<Ts&...> writers_;
  size_t size_;

 public:
  multi_writer(Ts&... writers)
      : writers_{writers...}
      , size_{sizeof... (writers)} {}

  std::pair<int, std::error_code> write(std::span<uint8_t const> p) {
    using io::write;
    for (auto i = 0; i < static_cast<int>(size_); ++i) {
      auto [n, err] = detail::visit_at([&](auto& w) {
        return write(w, p);
      }, writers_, i);
      if (err != nil) {
        return {n, err};
      }
      if (n != static_cast<int>(p.size())) {
        return {n, error::short_write};
      }
    }
    return {static_cast<int>(p.size()), nil};
  }

  std::pair<int, std::error_code> write_string(std::string_view s) {
    using io::write_string;
    for (auto i = 0; i < static_cast<int>(size_); ++i) {
      auto [n, err] = detail::visit_at([&](auto& w) {
        return write_string(w, s);
      });
      if (err != nil) {
        return {n, err};
      }
      if (n != static_cast<int>(s.size())) {
        return {n, error::short_write};
      }
    }
    return {static_cast<int>(s.size()), nil};
  }
};

}  // namespace bongo::io
