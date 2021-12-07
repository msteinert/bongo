// Copyright The Go Authors.

#pragma once

#include <array>
#include <iterator>
#include <span>
#include <string_view>

#include <bongo/bytes.h>
#include <bongo/fmt/detail/concepts.h>
#include <bongo/strconv.h>
#include <bongo/strings/builder.h>
#include <bongo/unicode/utf8.h>

namespace bongo::fmt::detail {

constexpr static std::string_view ldigits = "0123456789abcdefx";
constexpr static std::string_view udigits = "0123456789ABCDEFX";

struct fmt_flags {
  bool wid_present = false;
  bool prec_present = false;
  bool minus = false;
  bool plus = false;
  bool sharp = false;
  bool space = false;
  bool zero = false;

  bool plus_v = false;
  bool sharp_v = false;
};

class fmt {
  std::array<uint8_t, 68> intbuf_;

 public:
  fmt_flags flags = fmt_flags{};
  int wid;
  int prec;

  auto clear_flags() noexcept -> void;
  auto write_padding(bytes::buffer& out, int n, bool zero) const -> void;
  auto write_padding(bytes::buffer& out, int n) const -> void;
  auto pad(bytes::buffer& out, std::span<uint8_t> b) const -> void;
  auto pad(bytes::buffer& out, std::string_view s) const -> void;
  auto truncate(std::string_view s) const noexcept -> std::string_view;
  auto truncate(nullptr_t) const noexcept -> std::string_view { return std::string_view{}; }

  template <Boolean T>
  auto format(T&& v, bytes::buffer& out) -> void;

  template <Integer T>
  auto format(T&& v, bytes::buffer& out) -> void;

  template <Integer T>
  auto format(T&& v, int base, rune verb, std::string_view digits, bytes::buffer& out) -> void;

  template <Floating T>
  auto format(T&& v, bytes::buffer& out) -> void;

  template <Floating T>
  auto format(T&& v, rune verb, int precision, bytes::buffer& out) -> void;

  template <String T>
  auto format(T&& s, bytes::buffer& out) -> void;

  template <Bytes T>
  auto format(T&& s, bytes::buffer& out) -> void;

  template <String T>
  auto format(T&& s, std::string_view digits, bytes::buffer& out) -> void;

  template <Bytes T>
  auto format(T&& s, std::string_view digits, bytes::buffer& out) -> void;

  template <std::input_iterator In>
  auto format(In begin, In end, std::string_view digits, bytes::buffer& out) -> void;

  template <Integer T>
  auto format_c(T&& c, bytes::buffer& out) -> void;

  template <Integer T>
  auto format_qc(T&& c, bytes::buffer& out) -> void;

  template <Integer T>
  auto format_unicode(T&& u, bytes::buffer& out) -> void;

  template <String T>
  auto format_q(T&& s, bytes::buffer& out) -> void;
};

template <Boolean T>
auto fmt::format(T&& v, bytes::buffer& out) -> void {
  if (v) {
    pad(out, "true");
  } else {
    pad(out, "false");
  }
}

template <Integer T>
auto fmt::format(T&& v, bytes::buffer& out) -> void {
  format(v, 10, unicode::utf8::to_rune('d'), ldigits, out);
}

template <Integer T>
auto fmt::format(T&& v, int base, rune verb, std::string_view digits, bytes::buffer& out) -> void {
  auto negative = std::is_signed_v<std::remove_cvref_t<T>> && v < 0;
  uint64_t u = negative ? -static_cast<int64_t>(v) : v;

  auto buf = std::span<uint8_t>{intbuf_};
  std::vector<uint8_t> intbuf;
  if (flags.wid_present | flags.prec_present) {
    auto width = 3 + wid + prec;
    if (width > static_cast<int>(buf.size())) {
      intbuf.resize(width);
      buf = std::span{intbuf};
    }
  }

  auto precision = 0;
  if (flags.prec_present) {
    precision = prec;
    if (precision == 0 && v == 0) {
      write_padding(out, wid, false);
      return;
    }
  } else if (flags.zero && flags.wid_present) {
    precision = wid;
    if (negative || flags.plus || flags.space) {
      --precision;
    }
  }

  auto i = buf.size();
  switch (base) {
  case 10:
    while (u >= 10) {
      auto next = u / 10;
      buf[--i] = static_cast<uint8_t>('0' + u - next*10);
      u = next;
    }
    break;
  case 16:
    while (u >= 16) {
      buf[--i] = digits[u&0xf];
      u >>= 4;
    }
    break;
  case 8:
    while (u >= 8) {
      buf[--i] = static_cast<uint8_t>('0' + (u&7));
      u >>= 3;
    }
    break;
  case 2:
    while (u >= 2) {
      buf[--i] = static_cast<uint8_t>('0' + (u&1));
      u >>= 1;
    }
    break;
  default:
    throw std::logic_error{"fmt: unknown base; unreachable"};
  }

  buf[--i] = digits[u];
  while (i > 0 && precision > static_cast<int>(buf.size()-i)) {
    buf[--i] = '0';
  }

  if (flags.sharp) {
    switch (base) {
    case 2:
      buf[--i] = 'b';
      buf[--i] = '0';
      break;
    case 8:
      if (buf[i] != '0') {
        buf[--i] = '0';
      }
      break;
    case 16:
      buf[--i] = digits[16];
      buf[--i] = '0';
      break;
    default:
      break;
    }
  }
  if (verb == 'O') {
    buf[--i] = 'o';
    buf[--i] = '0';
  }

  if (negative) {
    buf[--i] = '-';
  } else if (flags.plus) {
    buf[--i] = '+';
  } else if (flags.space) {
    buf[--i] = ' ';
  }

  auto old_zero = flags.zero;
  flags.zero = false;
  pad(out, buf.subspan(i));
  flags.zero = old_zero;
}

template <Floating T>
auto fmt::format(T&& v, bytes::buffer& out) -> void {
  strconv::format(v, std::back_inserter(out));
}

template <Floating T>
auto fmt::format(T&& v, rune verb, int precision, bytes::buffer& out) -> void {
  if (flags.prec_present) {
    precision = prec;
  }

  // Format number, reserving space for leading + sign if needed.
  auto buf = bytes::buffer{};
  buf.write_byte('+');
  strconv::format(v, std::back_inserter(buf), verb, precision);
  auto num = buf.bytes();
  if (num[1] == '-' || num[1] == '+') {
    buf.read_byte();
    num = buf.bytes();
  }

  if (flags.space && num[0] == '+' && !flags.plus) {
    num[0] = ' ';
  }

  if (num[1] == 'I' || num[1] == 'N') {
    auto old_zero = flags.zero;
    flags.zero = false;
    if (num[1] == 'N' && !flags.space && !flags.plus) {
      num = num.subspan(1);
    }
    pad(out, num);
    flags.zero = old_zero;
    return;
  }

  if (flags.sharp && verb != 'b') {
    auto digits = 0;
    switch (verb) {
    case 'v': case 'g': case 'G': case 'x':
      digits = precision;
      if (digits == -1) {
        digits = 6;
      }
      break;
    default:
      break;
    }

    bytes::buffer tail;
    auto has_decimal_point = false;
    auto saw_nonzero_digit = false;

    for (size_t i = 1; i < num.size(); ++i) {
      switch (num[i]) {
      case '.':
        has_decimal_point = true;
        break;
      case 'p': case 'P':
        tail.write(num.subspan(i));
        buf.truncate(i);
        num = buf.bytes();
        break;
      case 'e': case 'E':
        if (verb != 'x' && verb != 'X') {
          tail.write(num.subspan(i));
          buf.truncate(i);
          num = buf.bytes();
          break;
        }
        [[fallthrough]];
      default:
        if (num[i] != '0') {
          saw_nonzero_digit = true;
        }
        if (saw_nonzero_digit == true) {
          --digits;
        }
      }
    }

    if (!has_decimal_point) {
      if (num.size() == 2 && num[1] == '0') {
        --digits;
      }
      buf.write_byte('.');
      num = buf.bytes();
    }

    while (digits > 0) {
      buf.write_byte('0');
      num = buf.bytes();
      --digits;
    }

    buf.write(tail);
    num = buf.bytes();
    if (num[1] == '-' || num[1] == '+') {
      num = num.subspan(1);
    }
  }

  if (flags.plus || num[0] != '+') {
    if (flags.zero && flags.wid_present && wid > static_cast<int>(num.size())) {
      out.write_byte(num[0]);
      write_padding(out, wid - num.size());
      out.write(num.subspan(1));
      return;
    }
    pad(out, num);
    return;
  }
  pad(out, num.subspan(1));
}

template <String T>
auto fmt::format(T&& s, bytes::buffer& out) -> void {
  pad(out, truncate(s));
}

template <Bytes T>
auto fmt::format(T&& s, bytes::buffer& out) -> void {
  format(std::begin(s), std::end(s), ldigits, out);
}

template <String T>
auto fmt::format(T&& s, std::string_view digits, bytes::buffer& out) -> void {
  format(std::begin(s), std::end(s), digits, out);
}

template <Bytes T>
auto fmt::format(T&& s, std::string_view digits, bytes::buffer& out) -> void {
  format(std::begin(s), std::end(s), digits, out);
}

template <std::input_iterator In>
auto fmt::format(In begin, In end, std::string_view digits, bytes::buffer& out) -> void {
  auto length = std::distance(begin, end);
  if (flags.prec_present && prec < length) {
    length = prec;
    end = std::next(begin, length);
  }
  auto width = 2 * length;
  if (width > 0) {
    if (flags.space) {
      if (flags.sharp) {
        width *= 2;
      }
      width += length - 1;
    } else if (flags.sharp) {
      width += 2;
    }
  } else {
    if (flags.wid_present) {
      write_padding(out, wid);
    }
    return;
  }
  if (flags.wid_present && wid > width && !flags.minus) {
    write_padding(out, wid - width);
  }
  if (flags.sharp) {
    out.write_byte('0');
    out.write_byte(digits[16]);
  }
  uint8_t c;
  for (auto it = begin; it < end; ++it) {
    if (flags.space && it > begin) {
      out.write_byte(' ');
      if (flags.sharp) {
        out.write_byte('0');
        out.write_byte(digits[16]);
      }
    }
    c = *it;
    out.write_byte(digits[c>>4]);
    out.write_byte(digits[c&0xf]);
  }
  if (flags.wid_present && wid > width && flags.minus) {
    write_padding(out, wid - width);
  }
}

template <Integer T>
auto fmt::format_c(T&& c, bytes::buffer& out) -> void {
  auto r = unicode::utf8::to_rune(c);
  if (!unicode::utf8::valid(r)) {
    r = unicode::utf8::rune_error;
  }
  auto w = unicode::utf8::encode(r, intbuf_.begin());
  pad(out, std::span<uint8_t>{intbuf_.begin(), w});
}

template <Integer T>
auto fmt::format_qc(T&& c, bytes::buffer& out) -> void {
  auto r = unicode::utf8::to_rune(c);
  if (!unicode::utf8::valid(r)) {
    r = unicode::utf8::rune_error;
  }
  decltype(intbuf_)::iterator end;
  if (flags.plus) {
    end = strconv::quote_to_ascii(r, intbuf_.begin());
  } else {
    end = strconv::quote(r, intbuf_.begin());
  }
  pad(out, std::span<uint8_t>{intbuf_.begin(), end});
}

template <Integer T>
auto fmt::format_unicode(T&& v, bytes::buffer& out) -> void {
  auto u = static_cast<uint64_t>(v);
  auto precision = 4;
  std::vector<uint8_t> intbuf;
  auto buf = std::span<uint8_t>{intbuf_};

  if (flags.prec_present && prec > 4) {
    precision = prec;
    auto width = 2 + precision + 2 + unicode::utf8::utf_max + 1;
    if (width > static_cast<int>(buf.size())) {
      intbuf.resize(width);
      buf = std::span{intbuf};
    }
  }

  auto i = buf.size();
  auto r = unicode::utf8::to_rune(u);

  if (flags.sharp && r <= unicode::utf8::max_rune && strconv::is_print(r)) {
    buf[--i] = '\'';
    i -= unicode::utf8::len(r);
    unicode::utf8::encode(r, std::next(buf.begin(), i));
    buf[--i] = '\'';
    buf[--i] = ' ';
  }

  while (u >= 16) {
    buf[--i] = udigits[u&0xf];
    --precision;
    u >>= 4;
  }

  buf[--i] = udigits[u];
  --precision;

  while (precision > 0) {
    buf[--i] = '0';
    --precision;
  }

  buf[--i] = '+';
  buf[--i] = 'U';

  auto old_zero = flags.zero;
  flags.zero = false;
  pad(out, buf.subspan(i));
  flags.zero = old_zero;
}

template <String T>
auto fmt::format_q(T&& s, bytes::buffer& out) -> void {
  s = truncate(s);
  if (flags.sharp && strconv::can_rawquote(s)) {
    auto builder = strings::builder{};
    builder.grow(3 + s.size() + 2);
    builder.write_string("R\"(");
    builder.write_string(s);
    builder.write_string(")\"");
    pad(out, bytes::to_string(builder.bytes()));
    return;
  }
  if (flags.plus) {
    pad(out, strconv::quote_to_ascii(s));
  } else {
    pad(out, strconv::quote(s));
  }
}

}  // namespace bongo::fmt::detail
