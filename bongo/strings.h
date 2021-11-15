// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <string_view>
#include <utility>

#include <bongo/detail/bytealg.h>
#include <bongo/error.h>

namespace bongo::strings {

/// Returns the index of the first instance of substr in s, or
/// std::string_view::npos if substr is not present in s.
inline std::string_view::size_type index(std::string_view s, std::string_view substr) {
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
    if (fails >= 4+i>>4 && i < t) {
      auto j = detail::bytealg::index_rabin_karp(s.substr(i), substr);
      if (j == std::string_view::npos) {
        return std::string_view::npos;
      }
      return i + j;
    }
  }
  return std::string_view::npos;
}

/// Reports whether substr is within s.
inline bool contains(std::string_view s, std::string_view substr) {
  return index(s, substr) != std::string_view::npos;
}

/// Tests whether the string s begins with prefix.
inline bool has_prefix(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

/// Tests whether the string s ends with suffix.
inline bool has_suffix(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() && s.substr(s.size()-suffix.size()) == suffix;
}

}  // namesapce bongo::strings
