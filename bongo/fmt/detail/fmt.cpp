// Copyright The Go Authors.

#include <string_view>

#include <bongo/bytes/buffer.h>
#include <bongo/fmt/detail/fmt.h>
#include <bongo/unicode/utf8.h>

namespace bongo::fmt::detail {

auto fmt::clear_flags() noexcept -> void {
  flags = fmt_flags{};
}

auto fmt::write_padding(bytes::buffer& out, long n, bool zero) const -> void {
  if (n <= 0) {
  } else {
    auto pad_byte = !zero ? ' ' : '0';
    for (auto i = 0; i < n; ++i) {
      out.write_byte(pad_byte);
    }
  }
}

auto fmt::write_padding(bytes::buffer& out, long n) const -> void {
  write_padding(out, n, flags.zero);
}

auto fmt::pad(bytes::buffer& out, std::span<uint8_t> b) const -> void {
  if (!flags.wid_present || wid == 0) {
    out.write(b);
  } else {
    auto width = wid - unicode::utf8::count(b.begin(), b.end());
    if (!flags.minus) {
      // left padding
      write_padding(out, width);
      out.write(b);
    } else {
      // right padding
      out.write(b);
      write_padding(out, width);
    }
  }
}

auto fmt::pad(bytes::buffer& out, std::string_view s) const -> void {
  if (!flags.wid_present || wid == 0) {
    out.write_string(s);
  } else {
    auto width = wid - unicode::utf8::count(s.begin(), s.end());
    if (!flags.minus) {
      // left padding
      write_padding(out, width);
      out.write_string(s);
    } else {
      // right padding
      out.write_string(s);
      write_padding(out, width);
    }
  }
}

auto fmt::truncate(std::string_view s) const noexcept -> std::string_view {
  if (flags.prec_present) {
    auto n = prec;
    for (auto [i, r] : unicode::utf8::range{s}) {
      if (--n < 0) {
        return s.substr(0, i);
      }
    }
  }
  return s;
}

}  // namespace bongo::fmt::detail
