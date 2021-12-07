// Copyright The Go Authors.

#pragma once

#include <string_view>
#include <utility>

#include <bongo/detail/bytealg.h>
#include <bongo/strings/builder.h>
#include <bongo/unicode/utf8.h>

namespace bongo::strings {

// Returns the index of the first instance of substr in s, or
// std::string_view::npos if substr is not present in s.
constexpr std::string_view::size_type index(std::string_view s, std::string_view substr) {
  if (substr.size() == 0) {
    return 0;
  } else if (substr.size() == 1) {
    return s.find_first_of(substr.front());
  } else if (substr.size() == s.size()) {
    return substr == s ? 0 : std::string_view::npos;
  } else if (substr.size() > s.size()) {
    return std::string_view::npos;
  }
  auto c0 = substr[0];
  auto c1 = substr[1];
  std::string_view::size_type i = 0, fails = 0;
  auto t = s.size() - substr.size() + 1;
  while (i < t) {
    if (s[i] != c0) {
      auto o = s.substr(i+1).find_first_of(c0);
      if (o == std::string_view::npos) {
        return std::string_view::npos;
      }
      i += o + 1;
    }
    if (s[i+1] == c1 && s.substr(i, substr.size()) == substr) {
      return i;
    }
    ++i;
    ++fails;
    if (fails >= (4+i)>>4 && i < t) {
      auto j = detail::bytealg::index_rabin_karp(s.substr(i), substr);
      if (j == std::string_view::npos) {
        return std::string_view::npos;
      }
      return i + j;
    }
  }
  return std::string_view::npos;
}

// Returns the index of the first instance of c in s, or std::string_view::npos
// if c is not present is s.
constexpr std::string_view::size_type index(std::string_view s, uint8_t c) {
  return s.find_first_of(c);
}

// Returns the index of the first instance of the Unicode code point r, or
// std::string_view::npos if rune is not present in s.
constexpr std::string_view::size_type index(std::string_view s, rune r) {
  namespace utf8 = unicode::utf8;
  if (0 <= r && r < utf8::rune_self) {
    return index(s, static_cast<uint8_t>(r));
  } else if (r == utf8::rune_error) {
    for (auto [i, r] : utf8::range{s}) {
      if (r == utf8::rune_error) {
        return i;
      }
    }
    return std::string_view::npos;
  } else if (!utf8::valid(r)) {
    return std::string_view::npos;
  } else {
    return index(s, utf8::encode(r));
  }
}

// Reports whether substr is within s.
constexpr bool contains(std::string_view s, std::string_view substr) {
  return index(s, substr) != std::string_view::npos;
}

// Reports whether the Unicode code point r is within s.
constexpr bool contains(std::string_view s, rune r) {
  return index(s, r) != std::string_view::npos;
}

// Tests whether the string s begins with prefix.
constexpr bool has_prefix(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

// Tests whether the string s ends with suffix.
constexpr bool has_suffix(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() && s.substr(s.size()-suffix.size()) == suffix;
}

// Repeat returns a new string consisting of count copies of the string s.
inline std::string repeat(std::string_view s, size_t count) {
  if (count == 0) {
    return "";
  }
  auto n = s.size() * count;
  auto b = strings::builder{};
  b.grow(n);
  b.write_string(s);
  while (b.size() < n) {
    if (b.size() <= n/2) {
      b.write(b.bytes());
    } else {
      b.write(b.bytes().subspan(0, n-b.size()));
    }
  }
  return std::string{b.str()};
}

}  // namesapce bongo::strings
