// Copyright The Go Authors.

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <bongo/detail/poll.h>
#include <bongo/detail/syscall/unix.h>
#include <bongo/os/types.h>

namespace bongo::os {

class file {
  std::unique_ptr<detail::poll::fd> pfd_;
  std::string name_;
  [[maybe_unused]] bool stdout_or_err_;
  [[maybe_unused]] bool append_mode_;

 public:
  file() = default;
  file(std::unique_ptr<detail::poll::fd> fd, std::string&& name, bool stdout_or_err, bool append_mode)
      : pfd_{std::move(fd)}
      , name_{std::move(name)}
      , stdout_or_err_{stdout_or_err}
      , append_mode_{append_mode} {}
  file(file const& other) = delete;
  file& operator=(file const& other) = delete;
  file(file&& other) = default;
  file& operator=(file&& other) = default;
  ~file() = default;

  std::string const& name() const { return name_; }
  std::error_code close() { return pfd_->close(); }
  uintptr_t fd() const noexcept { return static_cast<uintptr_t>(pfd_->sysfd); }

  std::pair<file_info, std::error_code> stat();
};

std::pair<file, std::error_code> make_file(uintptr_t fd, std::string&& name);
std::pair<file, std::error_code> make_file(uintptr_t fd, std::string const& name);

std::pair<file, std::error_code> open_file_nolog(std::string&& name, long flag, file_mode perm);
std::pair<file, std::error_code> open_file_nolog(std::string const& name, long flag, file_mode perm);

}  // namespace bongo::os
