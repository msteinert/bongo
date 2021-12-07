// Copyright The Go Authors.

#pragma once

#include <string>
#include <string_view>

namespace bongo::io::fs {

class file_mode {
  uint32_t m_;

 public:
  static constexpr uint32_t dir         = 0x80000000;
  static constexpr uint32_t append      = 0x40000000;
  static constexpr uint32_t exclusive   = 0x20000000;
  static constexpr uint32_t temporary   = 0x10000000;
  static constexpr uint32_t symlink     = 0x08000000;
  static constexpr uint32_t device      = 0x04000000;
  static constexpr uint32_t named_pipe  = 0x02000000;
  static constexpr uint32_t socket      = 0x01000000;
  static constexpr uint32_t setuid      = 0x00800000;
  static constexpr uint32_t setgid      = 0x00400000;
  static constexpr uint32_t char_device = 0x00200000;
  static constexpr uint32_t sticky      = 0x00100000;
  static constexpr uint32_t irregular   = 0x00080000;

  static constexpr uint32_t type_mask = dir | symlink | named_pipe | socket | device | char_device | irregular;
  static constexpr uint32_t perm_mask = 0777;

  constexpr file_mode(uint32_t m)
      : m_{m} {}

  operator uint32_t() { return m_; }
  constexpr file_mode operator|(file_mode const& other) const noexcept { return m_ | other.m_; }
  constexpr file_mode operator&(file_mode const& other) const noexcept { return m_ & other.m_; }

  constexpr file_mode& operator|=(file_mode const& other) noexcept {
    m_ |= other.m_;
    return *this;
  }

  constexpr file_mode& operator&=(file_mode const& other) noexcept {
    m_ &= other.m_;
    return *this;
  }

  std::string str() const {
    constexpr static std::string_view s = "dalTLDpSugct?";
    char buf[32] = {'\0'};
    size_t w = 0;
    for (size_t i = 0; i < s.size(); ++i) {
      if ((m_&(1<<static_cast<uint32_t>(32-1-i))) != 0) {
        buf[w++] = s[i];
      }
    }
    if (w == 0) {
      buf[w++] = '-';
    }
    constexpr static std::string_view rwx = "rwxrwxrwx";
    for (size_t i = 0; i < rwx.size(); ++i) {
      if ((m_&(1<<static_cast<uint32_t>(9-1-i))) != 0) {
        buf[w++] = rwx[i];
      } else {
        buf[w++] = '-';
      }
    }
    return std::string{buf, w};
  }

  constexpr bool is_dir() const noexcept {
    return (m_&dir) != 0;
  }

  constexpr bool is_regular() const noexcept {
    return (m_&type_mask) == 0;
  }

  constexpr file_mode perm() const noexcept {
    return m_ & perm_mask;
  }

  constexpr file_mode type() const noexcept {
    return m_ & type_mask;
  }
};

constexpr static file_mode mode_dir         = file_mode{file_mode::dir};
constexpr static file_mode mode_append      = file_mode{file_mode::append};
constexpr static file_mode mode_exclusive   = file_mode{file_mode::exclusive};
constexpr static file_mode mode_temporary   = file_mode{file_mode::temporary};
constexpr static file_mode mode_symlink     = file_mode{file_mode::symlink};
constexpr static file_mode mode_device      = file_mode{file_mode::device};
constexpr static file_mode mode_named_pipe  = file_mode{file_mode::named_pipe};
constexpr static file_mode mode_socket      = file_mode{file_mode::socket};
constexpr static file_mode mode_setuid      = file_mode{file_mode::setuid};
constexpr static file_mode mode_setgid      = file_mode{file_mode::setgid};
constexpr static file_mode mode_char_device = file_mode{file_mode::char_device};
constexpr static file_mode mode_sticky      = file_mode{file_mode::sticky};
constexpr static file_mode mode_irregular   = file_mode{file_mode::irregular};

constexpr static file_mode mode_type =
  mode_dir | mode_symlink | mode_named_pipe | mode_socket | mode_device | mode_char_device | mode_irregular;

constexpr static file_mode mode_perm = 0777;

}  // namespace bongo::io::fs
