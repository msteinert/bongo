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

 public:
  multi_reader(Ts&... readers)
      : readers_{readers...} {}

  std::pair<long, std::error_code> read(std::span<uint8_t> p) {
    using io::read;
    for (size_t i = 0; i < sizeof...(Ts);) {
      auto [n, err] = detail::visit_at([&](auto& r) {
        return read(r, p);
      }, readers_, i);
      if (err == eof) {
        ++i;
      }
      if (n > 0 || err != eof) {
        if (err == eof && i < sizeof...(Ts)) {
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

 public:
  multi_writer(Ts&... writers)
      : writers_{writers...} {}

  std::pair<long, std::error_code> write(std::span<uint8_t const> p) {
    using io::write;
    for (size_t i = 0; i < sizeof...(Ts); ++i) {
      auto [n, err] = detail::visit_at([&](auto& w) {
        return write(w, p);
      }, writers_, i);
      if (err != nil) {
        return {n, err};
      }
      if (n != static_cast<long>(p.size())) {
        return {n, error::short_write};
      }
    }
    return {static_cast<long>(p.size()), nil};
  }

  std::pair<long, std::error_code> write_string(std::string_view s) {
    using io::write_string;
    for (size_t i = 0; i < sizeof...(Ts); ++i) {
      auto [n, err] = detail::visit_at([&](auto& w) {
        return write_string(w, s);
      }, writers_, i);
      if (err != nil) {
        return {n, err};
      }
      if (n != static_cast<long>(s.size())) {
        return {n, error::short_write};
      }
    }
    return {static_cast<long>(s.size()), nil};
  }
};

}  // namespace bongo::io
