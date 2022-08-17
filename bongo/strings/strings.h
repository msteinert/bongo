// Copyright The Go Authors.

#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <bongo/detail/bytealg.h>
#include <bongo/strings/builder.h>
#include <bongo/strings/detail/ascii_set.h>
#include <bongo/unicode/utf8.h>

#include <iostream>

namespace bongo::strings {

// Returns the index of the first instance of substr in s, or
// std::string_view::npos if substr is not present in s.
constexpr auto index(std::string_view s, std::string_view substr) -> std::string_view::size_type;

// Returns the index of the first instance of c in s, or std::string_view::npos
// if c is not present is s.
constexpr auto index(std::string_view s, uint8_t c) -> std::string_view::size_type;

// Returns the index of the first instance of the Unicode code point r, or
// std::string_view::npos if rune is not present in s.
constexpr auto index(std::string_view s, rune r) -> std::string_view::size_type;

constexpr auto index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type;

constexpr auto last_index(std::string_view s, std::string_view substr) -> std::string_view::size_type;

constexpr auto last_index(std::string_view s, uint8_t c) -> std::string_view::size_type;

constexpr auto last_index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type;

auto split(std::string_view s, std::string_view sep, int n) -> std::vector<std::string_view>;

auto split_after(std::string_view s, std::string_view sep, int n) -> std::vector<std::string_view>;

auto split(std::string_view s, std::string_view sep) -> std::vector<std::string_view>;

auto split_after(std::string_view s, std::string_view sep) -> std::vector<std::string_view>;

auto join(std::span<std::string_view> e, std::string_view sep) -> std::string;

// Counts the number of non-overlapping instances of substr in s. If substr is
// an empty string, Count returns 1 + the number of Unicode code points in s.
constexpr auto count(std::string_view s, std::string_view substr) -> int;

// Reports whether substr is within s.
template <typename T>
constexpr auto contains(std::string_view s, T substr) -> bool;

// Tests whether the string s begins with prefix.
constexpr auto has_prefix(std::string_view s, std::string_view prefix) -> bool;

// Tests whether the string s ends with suffix.
constexpr auto has_suffix(std::string_view s, std::string_view suffix) -> bool;

// Repeat returns a new string consisting of count copies of the string s.
auto repeat(std::string_view s, size_t count) -> std::string;

auto replace(std::string_view s, std::string_view old_s, std::string_view new_s, int n) -> std::string;

auto replace(std::string_view s, std::string_view old_s, std::string_view new_s) -> std::string;

constexpr auto cut(std::string_view s, std::string_view sep) -> std::tuple<std::string_view, std::string_view, bool>;

constexpr auto index(std::string_view s, std::string_view substr) -> std::string_view::size_type {
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
      auto j = bongo::detail::bytealg::index_rabin_karp(s.substr(i), substr);
      if (j == std::string_view::npos) {
        return std::string_view::npos;
      }
      return i + j;
    }
  }
  return std::string_view::npos;
}

constexpr auto index(std::string_view s, uint8_t c) -> std::string_view::size_type {
  return s.find_first_of(c);
}

constexpr auto index(std::string_view s, rune r) -> std::string_view::size_type {
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

constexpr auto index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type {
  namespace utf8 = unicode::utf8;
  if (chars.empty()) {
    return std::string_view::npos;
  }
  if (chars.size() == 1) {
    auto r = static_cast<rune>(chars[0]);
    if (r >= utf8::rune_self) {
      r = utf8::rune_error;
    }
    return index(s, r);
  }
  if (s.size() > 8) {
    if (auto [as, is_ascii] = detail::make_ascii_set(chars); is_ascii) {
      for (std::string_view::size_type i = 0; i < s.size(); ++i) {
        if (as.contains(s[i])) {
          return i;
        }
      }
      return std::string_view::npos;
    }
  }
  for (auto [i, r] : utf8::range{s}) {
    if (index(chars, r) != std::string_view::npos) {
      return i;
    }
  }
  return std::string_view::npos;
}

constexpr auto last_index(std::string_view s, std::string_view substr) -> std::string_view::size_type {
  if (substr.size() == 0) {
    return s.size();
  } else if (substr.size() == 1) {
    return s.find_last_of(substr.front());
  } else if (substr.size() == s.size()) {
    if (substr == s) {
      return 0;
    }
    return std::string_view::npos;
  } else if (substr.size() > s.size()) {
    return std::string_view::npos;
  }
  auto [hashss, pow] = bongo::detail::bytealg::hash_reverse(substr);
  auto last = s.size() - substr.size();
  uint32_t h = 0;
  for (auto i = s.size() - 1; i >= last; --i) {
    h = h*bongo::detail::bytealg::prime_rk + static_cast<uint32_t>(s[i]);
  }
  if (h == hashss && s.substr(last) == substr) {
    return last;
  }
  for (auto i = static_cast<int>(last) - 1; i >= 0; --i) {
    h *= bongo::detail::bytealg::prime_rk;
    h += static_cast<uint32_t>(s[i]);
    h -= pow * static_cast<uint32_t>(s[i+substr.size()]);
    if (h == hashss && s.substr(i, substr.size()) == substr) {
      return i;
    }
  }
  return std::string_view::npos;
}

constexpr auto last_index(std::string_view s, uint8_t c) -> std::string_view::size_type {
  return s.find_last_of(c);
}

constexpr auto last_index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type {
  namespace utf8 = unicode::utf8;
  if (chars.empty()) {
    return std::string_view::npos;
  }
  if (s.size() == 1) {
    auto r = static_cast<rune>(s[0]);
    if (r >= utf8::rune_self) {
      r = utf8::rune_error;
    }
    if (index(chars, r) != std::string_view::npos) {
      return 0;
    }
    return std::string_view::npos;
  }
  if (s.size() > 8) {
    if (auto [as, is_ascii] = detail::make_ascii_set(chars); is_ascii) {
      for (int i = s.size() - 1; i >= 0; --i) {
        if (as.contains(s[i])) {
          return i;
        }
      }
      return std::string_view::npos;
    }
  }
  if (chars.size() == 1) {
    auto r = static_cast<rune>(chars[0]);
    if (r >= utf8::rune_self) {
      r = utf8::rune_error;
    }
    for (int i = s.size(); i > 0;) {
      auto [rc, size] = utf8::decode_last(s.substr(0, i));
      i -= size;
      if (rc == r) {
        return i;
      }
    }
    return std::string_view::npos;
  }
  for (int i = s.size(); i > 0;) {
    auto [r, size] = utf8::decode_last(s.substr(0, i));
    i -= size;
    if (index(chars, r) != std::string_view::npos) {
      return i;
    }
  }
  return std::string_view::npos;
}

constexpr auto count(std::string_view s, std::string_view substr) -> int {
  namespace utf8 = unicode::utf8;
  if (substr.size() == 0) {
    return utf8::count(std::begin(s), std::end(s)) + 1;
  } else if (substr.size() == 1) {
    int n = 0;
    for (auto c : s) {
      if (c == substr[0]) {
        ++n;
      }
    }
    return n;
  }
  int n = 0;
  for (;;) {
    auto i = index(s, substr);
    if (i == std::string_view::npos) {
      return n;
    }
    ++n;
    s = s.substr(i+substr.size());
  }
}

template <typename T>
constexpr auto contains(std::string_view s, T substr) -> bool {
  return index(s, substr) != std::string_view::npos;
}

constexpr auto has_prefix(std::string_view s, std::string_view prefix) -> bool {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

constexpr auto has_suffix(std::string_view s, std::string_view suffix) -> bool {
  return s.size() >= suffix.size() && s.substr(s.size()-suffix.size()) == suffix;
}

constexpr auto cut(std::string_view s, std::string_view sep) -> std::tuple<std::string_view, std::string_view, bool> {
  if (auto i = index(s, sep); i != std::string_view::npos) {
    return {s.substr(0, i), s.substr(i+sep.size()), true};
  }
  return {s, "", false};
}

}  // namesapce bongo::strings
