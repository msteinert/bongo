// Copyright The Go Authors.

#pragma once

#include <semaphore>
#include <string_view>
#include <system_error>

#include <bongo/detail/poll/fd_mutex.h>
#include <bongo/syscall.h>

namespace bongo::detail::poll {

class fd {
  // Lock sysfd and serialize access to read and write.
  fd_mutex fdmu_;

  // Whether this is a file rather than a network socket.
  bool is_file_ = false;

  // Signaled when file is closed.
  std::binary_semaphore csema_{0};

  // True if this file has been set to blocking mode.
  bool is_blocking_ = false;

 public:
  // System file descriptor.
  int sysfd;

  // Whether this is a streaming descriptor, as opposed to a packet-based
  // descriptor like a UDP socket.
  bool is_stream;

  // Wheter a zero byte read indicates EOF. This is false for a message based
  // socket connection.
  bool zero_read_is_eof;

  fd(int sysfd_,
     bool is_stream_ = false,
     bool zero_read_is_eof_ = false)
      : sysfd{sysfd_}
      , is_stream{is_stream_}
      , zero_read_is_eof{zero_read_is_eof_} {}
  fd(fd const& other) = delete;
  fd& operator=(fd const& other) = delete;
  fd(fd&& other) = delete;
  fd& operator=(fd&& other) = delete;
  ~fd() { close(); }

  std::error_code init(std::string_view net, bool pollable);
  std::error_code close();
  std::error_code destroy();
  std::error_code fstat(struct ::stat* s);

 private:
  std::error_code incref();
  std::error_code decref();
};

}  // namespace bongo::detail::poll
