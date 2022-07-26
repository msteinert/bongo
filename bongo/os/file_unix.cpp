// Copyright The Go Authors.

#include <unistd.h>

#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "bongo/bongo.h"
#include "bongo/os/file_unix.h"
#include "bongo/os/types.h"
#include "bongo/syscall.h"

namespace bongo::os {
namespace {

enum class new_file_kind {
  new_file = 0,
  open_file,
  pipe,
  non_block,
};

std::pair<file, std::error_code> make_file(uintptr_t fd, std::string&& name, new_file_kind kind, bool append_mode = false) {
  auto fdi = static_cast<int>(fd);
  if (fdi < 0) {
    return {file{}, std::make_error_code(std::errc::invalid_argument)};
  }
  auto pfd = std::make_unique<detail::poll::fd>(fdi, true, true);
  auto nonblock = fdi == 1 || fdi == 2;
  auto pollable = kind == new_file_kind::open_file || kind == new_file_kind::pipe || kind == new_file_kind::non_block;
  if (auto err = pfd->init("file", pollable); err) {
    // This error indicates a failure to register with netpoll, real errors
    // will show up in later I/O.
  } else if (pollable) {
    if (auto err = syscall::set_nonblock(fdi, true); err == nil) {
      nonblock = true;
    }
  }

  return {file{std::move(pfd), std::move(name), nonblock, append_mode}, nil};
}

template <typename Fn>
std::error_code ignoring_interrupted(Fn fn) {
  for (;;) {
    auto err = fn();
    if (err != std::errc::interrupted) {
      return err;
    }
  }
}

std::string_view basename(std::string_view name) {
  int i = name.size() - 1;
  // Remove trailing slashes
  for (; i > 0 && name[i] == '/'; --i) {
    name = name.substr(0, i);
  }
  // Remove leading directory name
  for (--i; i >= 0; --i) {
    if (name[i] == '/') {
      name = name.substr(i+1);
      break;
    }
  }
  return name;
}

std::chrono::system_clock::time_point to_duration(time_t t) {
  return std::chrono::system_clock::time_point{std::chrono::seconds{t}};
}

}  // namespace

std::pair<file_info, std::error_code> file::stat() {
  struct ::stat s;
  auto err = pfd_->fstat(&s);
  if (err) {
    return {file_info{}, err};
  }
  file_mode mode = s.st_mode & 0777;
  switch (s.st_mode & S_IFMT) {
  case S_IFBLK:
    mode |= mode_device;
    break;
  case S_IFCHR:
    mode |= mode_device | mode_char_device;
    break;
  case S_IFDIR:
    mode |= mode_dir;
    break;
  case S_IFIFO:
    mode |= mode_named_pipe;
    break;
  case S_IFLNK:
    mode |= mode_symlink;
    break;
  case S_IFREG:
  default:
    break;
  case S_IFSOCK:
    mode |= mode_socket;
    break;
  }
  if ((s.st_mode&S_ISGID) != 0) {
    mode |= mode_setgid;
  }
  if ((s.st_mode&S_ISUID) != 0) {
    mode |= mode_setuid;
  }
  if ((s.st_mode&S_ISVTX) != 0) {
    mode |= mode_sticky;
  }
  return {file_info{std::string{basename(name_)}, s.st_size, mode, to_duration(s.st_mtime)}, nil};
}

std::pair<file, std::error_code> make_file(uintptr_t fd, std::string&& name) {
  auto kind = new_file_kind::new_file;
  if (auto [nb, err] = detail::syscall::unix::is_nonblock(static_cast<int>(fd));
      err == nil && nb) {
    kind = new_file_kind::non_block;
  }
  return make_file(fd, std::move(name), kind);
}

std::pair<file, std::error_code> make_file(uintptr_t fd, std::string const& name) {
  return make_file(fd, std::string{name});
}

std::pair<file, std::error_code> open_file_nolog(std::string&& name, int flag, file_mode perm) {
  int r;
  for (;;) {
    std::error_code e;
    std::tie(r, e) = syscall::open(name, flag, perm);
    if (!e) {
      break;
    }
    if (std::errc::interrupted == e) {
      continue;
    }
    return {file{}, e};
  }
  return make_file(
      static_cast<uintptr_t>(r),
      std::move(name),
      new_file_kind::open_file,
      (flag&o_append) != 0);
}

std::pair<file, std::error_code> open_file_nolog(std::string const& name, int flag, file_mode perm) {
  return open_file_nolog(std::string{name}, flag, perm);
}

}  // namespace bongo::os
