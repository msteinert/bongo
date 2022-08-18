// Copyright The Go Authors.

#pragma once

#include <string_view>

#include <bongo/detail/bytealg.h>
#include <bongo/strings/concepts.h>
#include <bongo/strings/detail/ascii_set.h>
#include <bongo/strings/detail/index.h>
#include <bongo/unicode/utf8.h>

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

template <IndexFunction T>
constexpr auto index(std::string_view s, T&& func) -> std::string_view::size_type;

constexpr auto index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type;

constexpr auto last_index(std::string_view s, std::string_view substr) -> std::string_view::size_type;

constexpr auto last_index(std::string_view s, uint8_t c) -> std::string_view::size_type;

template <IndexFunction T>
constexpr auto last_index(std::string_view s, T&& func) -> std::string_view::size_type;

constexpr auto last_index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type;

// Reports whether substr is within s.
template <typename T>
constexpr auto contains(std::string_view s, T substr) -> bool;

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

template <IndexFunction T>
constexpr auto index(std::string_view s, T&& func) -> std::string_view::size_type {
  return detail::index(s, std::move(func), true);
}

constexpr auto index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type {
  namespace utf8 = unicode::utf8;
  if (chars.empty()) {
    return std::string_view::npos;
  }
  if (chars.size() == 1) {
    auto r = utf8::to_rune(chars[0]);
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

template <IndexFunction T>
constexpr auto last_index(std::string_view s, T&& func) -> std::string_view::size_type {
  return detail::last_index(s, std::move(func), true);
}

constexpr auto last_index_any(std::string_view s, std::string_view chars) -> std::string_view::size_type {
  namespace utf8 = unicode::utf8;
  if (chars.empty()) {
    return std::string_view::npos;
  }
  if (s.size() == 1) {
    auto r = utf8::to_rune(s[0]);
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
    auto r = utf8::to_rune(chars[0]);
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

template <typename T>
constexpr auto contains(std::string_view s, T substr) -> bool {
  return index(s, substr) != std::string_view::npos;
}

}  // namesapce bongo::strings
