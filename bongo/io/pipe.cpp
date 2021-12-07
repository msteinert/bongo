// Copyright The Go Authors.

#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <system_error>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "bongo/bongo.h"
#include "bongo/io/detail/once_error.h"
#include "bongo/io/io.h"
#include "bongo/io/pipe.h"

namespace bongo::io {

std::pair<int, std::error_code> pipe::read(std::span<uint8_t> b) {
  std::optional<std::monostate> d;
  switch (select(
    recv_select_case(done_, d),
    default_select_case()
  )) {
  case 0:
    return {0, read_close_error()};
  default:
    break;
  }

  int nr = 0;
  std::optional<std::vector<uint8_t>> bw;
  switch (select(
    recv_select_case(wr_chan_, bw),
    recv_select_case(done_, d)
  )) {
  case 0:
    nr = static_cast<int>(std::min(bw->size(), b.size()));
    std::copy_n(bw->begin(), nr, b.begin());
    rd_chan_ << nr;
    return {nr, nil};
  case 1:
  default:
    return {0, read_close_error()};
  }
}

std::error_code pipe::close_read(std::error_code err) {
  if (err == nil) {
    err = error::closed_pipe;
  }
  rd_err_.store(err);
  std::call_once(once_, [this]() { done_.close(); });
  return nil;
}

std::pair<int, std::error_code> pipe::write(std::span<uint8_t const> b) {
  auto lock = std::unique_lock<std::mutex>{wr_mutex_, std::defer_lock};
  std::optional<std::monostate> d;
  switch (select(
    recv_select_case(done_, d),
    default_select_case()
  )) {
  case 0:
    return {0, write_close_error()};
  default:
    lock.lock();
    break;
  }

  int n = 0;
  for (auto once = true; once || b.size() > 0; once = false) {
    int nw;
    switch (select(
      send_select_case(wr_chan_, std::vector<uint8_t>{b.begin(), b.end()}),
      recv_select_case(done_, d)
    )) {
    case 0:
      nw << rd_chan_;
      b = b.subspan(nw);
      n += nw;
      break;
    case 1:
      return {n, write_close_error()};
    }
  }
  return {n, nil};
}

std::error_code pipe::close_write(std::error_code err) {
  if (err == nil) {
    err = io::eof;
  }
  wr_err_.store(err);
  std::call_once(once_, [this]() { done_.close(); });
  return nil;
}

std::error_code pipe::read_close_error() {
  auto rd_err = rd_err_.load();
  if (auto wr_err = wr_err_.load(); rd_err == nil && wr_err != nil) {
    return wr_err;
  }
  return error::closed_pipe;
}

std::error_code pipe::write_close_error() {
  auto wr_err = wr_err_.load();
  if (auto rd_err = rd_err_.load(); wr_err == nil && rd_err != nil) {
    return rd_err;
  }
  return error::closed_pipe;
}

}  // namespace bongo::io
