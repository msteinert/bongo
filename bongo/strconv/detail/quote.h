// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include <bongo/strconv/print.h>
#include <bongo/unicode/utf8.h>

namespace bongo::strconv::detail {

constexpr static std::string_view lower_hex = "0123456789abcdef";
constexpr static std::string_view upper_hex = "0123456789ABCDEF";

template <std::input_iterator It>
constexpr bool can_rawquote(It it, It end) {
  while (it < end) {
    if (*it == ')' && std::next(it) != end && *std::next(it) == '"') {
      return false;
    }
    auto [r, wid] = unicode::utf8::decode(it, end);
    it = std::next(it, wid);
    if (wid > 1) {
      if (r == 0xfeff) {
        return false;  // BOMs are invisible and should not be quoted.
      }
      continue;
    }
    if (r == unicode::utf8::rune_error) {
      return false;
    }
    if ((r < ' ' && r != '\t') || r == 0x007f) {
      return false;
    }
  }
  return true;
}

template <std::output_iterator<uint8_t> OutputIt>
OutputIt escape_rune(OutputIt out, rune r, bool ascii_only, bool graphic_only) {
  if (r == '\'' || r == '\\') {
    *out++ = '\'';
    *out++ = '\\';
    *out++ = static_cast<uint8_t>(r);
    *out++ = '\'';
    return out;
  }
  if (ascii_only) {
    if (r < unicode::utf8::rune_self && is_print(r)) {
      *out++ = '\'';
      *out++ = static_cast<uint8_t>(r);
      *out++ = '\'';
      return out;
    }
  } else if (is_print(r) || (graphic_only && is_in_graphic_list(r))) {
    if (r >= unicode::utf8::rune_self) {
      if (r < 0x10000) {
        *out++ = 'u';
      } else {
        *out++ = 'U';
      }
    }
    *out++ = '\'';
    out = unicode::utf8::encode(r, out);
    *out++ = '\'';
    return out;
  }
  switch (r) {
  case '\a':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 'a';
    break;
  case '\b':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 'b';
    break;
  case '\f':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 'f';
    break;
  case '\n':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 'n';
    break;
  case '\r':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 'r';
    break;
  case '\t':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 't';
    break;
  case '\v':
    *out++ = '\'';
    *out++ = '\\';
    *out++ = 'v';
    break;
  default:
    if (static_cast<uint64_t>(r) < ' ') {
      *out++ = '\'';
      *out++ = '\\';
      *out++ = 'x';
      *out++ = lower_hex[static_cast<uint8_t>(r)>>4];
      *out++ = lower_hex[static_cast<uint8_t>(r)&0xf];
    } else if (!unicode::utf8::valid(r)) {
      r = 0xfffd;
      *out++ = 'u';
      *out++ = '\'';
      *out++ = '\\';
      *out++ = 'u';
      for (auto s = 12; s >= 0; s -= 4) {
        *out++ = lower_hex[(r>>static_cast<unsigned>(s))&0xf];
      }
    } else if (static_cast<uint64_t>(r) < 0x10000) {
      *out++ = 'u';
      *out++ = '\'';
      *out++ = '\\';
      *out++ = 'u';
      for (auto s = 12; s >= 0; s -= 4) {
        *out++ = lower_hex[(r>>static_cast<unsigned>(s))&0xf];
      }
    } else {
      *out++ = 'U';
      *out++ = '\'';
      *out++ = '\\';
      *out++ = 'U';
      for (auto s = 28; s >= 0; s -= 4) {
        *out++ = lower_hex[(r>>static_cast<unsigned>(s))&0xf];
      }
    }
  }
  *out++ = '\'';
  return out;
}

template <std::output_iterator<uint8_t> OutputIt>
OutputIt escape_rune(OutputIt out, rune r, char quote, bool ascii_only, bool graphic_only) {
  if (r == static_cast<rune>(quote) || r == '\\') {
    *out++ = '\\';
    *out++ = static_cast<uint8_t>(r);
    return out;
  }
  if (ascii_only) {
    if (r < unicode::utf8::rune_self && is_print(r)) {
      *out++ = static_cast<uint8_t>(r);
      return out;
    }
  } else if (is_print(r) || (graphic_only && is_in_graphic_list(r))) {
    return unicode::utf8::encode(r, out);
  }
  *out++ = '\\';
  switch (r) {
  case '\a':
    *out++ = 'a';
    break;
  case '\b':
    *out++ = 'b';
    break;
  case '\f':
    *out++ = 'f';
    break;
  case '\n':
    *out++ = 'n';
    break;
  case '\r':
    *out++ = 'r';
    break;
  case '\t':
    *out++ = 't';
    break;
  case '\v':
    *out++ = 'v';
    break;
  default:
    if (r < ' ') {
      *out++ = 'x';
      *out++ = lower_hex[static_cast<uint8_t>(r)>>4];
      *out++ = lower_hex[static_cast<uint8_t>(r)&0xf];
      if (quote == '"') {
        *out++ = quote;
        *out++ = quote;
      }
    } else if (!unicode::utf8::valid(r)) {
      r = 0xfffd;
      *out++ = 'u';
      for (auto s = 12; s >= 0; s -= 4) {
        *out++ = lower_hex[(r>>static_cast<unsigned>(s))&0xf];
      }
    } else if (r < 0x10000) {
      *out++ = 'u';
      for (auto s = 12; s >= 0; s -= 4) {
        *out++ = lower_hex[(r>>static_cast<unsigned>(s))&0xf];
      }
    } else {
      *out++ = 'U';
      for (auto s = 28; s >= 0; s -= 4) {
        *out++ = lower_hex[(r>>static_cast<unsigned>(s))&0xf];
      }
    }
  }
  return out;
}

template <std::input_iterator InputIt, std::output_iterator<uint8_t> OutputIt>
OutputIt quote_with(InputIt it, InputIt end, OutputIt out, char quote, bool ascii_only, bool graphic_only) {
  *out++ = quote;
  while (it < end) {
    auto r = static_cast<rune>(static_cast<uint8_t>(*it));
    size_t width = 1;
    if (r >= unicode::utf8::rune_self) {
      std::tie(r, width) = unicode::utf8::decode(it, end);
    }
    if (width == 1 && r == unicode::utf8::rune_error) {
      *out++ = '\\';
      *out++ = 'x';
      *out++ = lower_hex[static_cast<uint8_t>(r)>>4];
      *out++ = lower_hex[static_cast<uint8_t>(r)&0xf];
      if (quote == '"') {
        *out++ = quote;
        *out++ = quote;
      }
    } else {
      escape_rune(out, r, quote, ascii_only, graphic_only);
    }
    it = std::next(it, width);
  }
  *out++ = quote;
  return out;
}

inline std::string quote_with(std::string_view in, char quote, bool ascii_only, bool graphic_only) {
  auto out = std::string{};
  out.reserve(3*in.size()/2);
  detail::quote_with(in.begin(), in.end(), std::back_inserter(out), quote, ascii_only, graphic_only);
  return out;
}

}  // namespace bongo::strconv::detail
