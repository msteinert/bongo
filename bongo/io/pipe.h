// Copyright The Go Authors.

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <system_error>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include <bongo/bongo.h>
#include <bongo/io/detail/once_error.h>
#include <bongo/io/io.h>

namespace bongo::io {

class pipe {
  std::mutex wr_mutex_;
  chan<std::vector<uint8_t>> wr_chan_;
  chan<int> rd_chan_;
  std::once_flag once_;
  chan<std::monostate> done_;
  detail::once_error rd_err_;
  detail::once_error wr_err_;

 public:
  pipe() = default;

  std::pair<int, std::error_code> read(std::span<uint8_t> b);
  std::error_code close_read(std::error_code err);
  std::pair<int, std::error_code> write(std::span<uint8_t const> b);
  std::error_code close_write(std::error_code err);

 private:
  std::error_code read_close_error();
  std::error_code write_close_error();
};

class pipe_reader {
  std::shared_ptr<pipe> p_;

 public:
  pipe_reader() = default;
  pipe_reader(std::shared_ptr<pipe> p)
      : p_{std::move(p)} {}

  std::pair<int, std::error_code> read(std::span<uint8_t> data) {
    return p_->read(data);
  }

  std::error_code close() {
    return close_with_error(nil);
  }

  std::error_code close_with_error(std::error_code err) {
    return p_->close_read(err);
  }
};

class pipe_writer {
  std::shared_ptr<pipe> p_;

 public:
  pipe_writer() = default;
  pipe_writer(std::shared_ptr<pipe> p)
      : p_{std::move(p)} {}

  std::pair<int, std::error_code> write(std::span<uint8_t const> data) {
    return p_->write(data);
  }

  std::error_code close() {
    return close_with_error(nil);
  }

  std::error_code close_with_error(std::error_code err) {
    return p_->close_write(err);
  }
};

inline std::pair<pipe_reader, pipe_writer> make_pipe() {
  auto p = std::make_shared<pipe>();
  return {pipe_reader{p}, pipe_writer{p}};
}

}  // namespace bongo::io
