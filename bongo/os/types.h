// Copyright The Go Authors.

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bongo/io/fs.h>
#include <bongo/syscall.h>

namespace bongo::os {

inline int getpagesize() noexcept {
  return syscall::getpagesize();
}

using io::fs::file_mode;

using io::fs::mode_dir;
using io::fs::mode_append;
using io::fs::mode_exclusive;
using io::fs::mode_temporary;
using io::fs::mode_symlink;
using io::fs::mode_device;
using io::fs::mode_named_pipe;
using io::fs::mode_socket;
using io::fs::mode_setuid;
using io::fs::mode_setgid;
using io::fs::mode_char_device;
using io::fs::mode_sticky;
using io::fs::mode_irregular;

using io::fs::mode_type;
using io::fs::mode_perm;

// Flags to open_file.
// Exactly one of o_rdonly, o_wronly, or o_rdwr must be specified.
constexpr static int o_rdonly = O_RDONLY;  // Open the file read-only.
constexpr static int o_wronly = O_WRONLY;  // Open the file write-only.
constexpr static int o_rdwr   = O_RDWR;    // open the file read-write.
// The remaining flags may be or'ed in to control the behavior.
constexpr static int o_append = O_APPEND;  // Append datat to the file when writing.
constexpr static int o_create = O_CREAT;   // Create a new file if no exists.
constexpr static int o_excl   = O_EXCL;    // Used with o_create, file must not exist.
constexpr static int o_sync   = O_SYNC;    // Open for synchronous I/O.
constexpr static int o_trunc  = O_TRUNC;   // Truncate regular writable file when opened.

struct file_info {
  std::string name;
  int64_t size = 0;
  file_mode mode = 0;
  std::chrono::system_clock::time_point mod_time;
};

}  // namespace bongo::os
