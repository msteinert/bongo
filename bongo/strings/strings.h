// Copyright The Go Authors.

#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <bongo/detail/bytealg.h>
#include <bongo/strings/builder.h>
#include <bongo/strings/concepts.h>
#include <bongo/strings/detail/ascii_set.h>
#include <bongo/strings/detail/trim.h>
#include <bongo/strings/index.h>
#include <bongo/unicode/utf8.h>

namespace bongo::strings {

constexpr auto trim(std::string_view s, std::string_view cutset) -> std::string_view;

constexpr auto trim_left(std::string_view s, std::string_view cutset) -> std::string_view;

template <typename T> requires IndexFunction<T>
constexpr auto trim_left(std::string_view s, T&& func) -> std::string_view;

constexpr auto trim_right(std::string_view s, std::string_view cutset) -> std::string_view;

template <typename T> requires IndexFunction<T>
constexpr auto trim_right(std::string_view s, T&& func) -> std::string_view;

constexpr auto trim_prefix(std::string_view s, std::string_view prefix) -> std::string_view;

constexpr auto trim_suffix(std::string_view s, std::string_view suffix) -> std::string_view;

auto split(std::string_view s, std::string_view sep, long n) -> std::vector<std::string_view>;

auto split_after(std::string_view s, std::string_view sep, long n) -> std::vector<std::string_view>;

auto split(std::string_view s, std::string_view sep) -> std::vector<std::string_view>;

auto split_after(std::string_view s, std::string_view sep) -> std::vector<std::string_view>;

auto join(std::span<std::string_view const> e, std::string_view sep) -> std::string;

// Counts the number of non-overlapping instances of substr in s. If substr is
// an empty string, Count returns 1 + the number of Unicode code points in s.
constexpr auto count(std::string_view s, std::string_view substr) -> long;

// Tests whether the string s begins with prefix.
constexpr auto has_prefix(std::string_view s, std::string_view prefix) -> bool;

// Tests whether the string s ends with suffix.
constexpr auto has_suffix(std::string_view s, std::string_view suffix) -> bool;

template <typename T> requires MapFunction<T>
auto map(T&& mapping, std::string_view s) -> std::string;

auto to_valid_utf8(std::string_view s, std::string_view replacement) -> std::string;

// Repeat returns a new string consisting of count copies of the string s.
auto repeat(std::string_view s, size_t count) -> std::string;

auto replace(std::string_view s, std::string_view old_s, std::string_view new_s, long n) -> std::string;

auto replace(std::string_view s, std::string_view old_s, std::string_view new_s) -> std::string;

constexpr auto cut(std::string_view s, std::string_view sep) -> std::tuple<std::string_view, std::string_view, bool>;

constexpr auto trim(std::string_view s, std::string_view cutset) -> std::string_view {
  namespace utf8 = unicode::utf8;
  if (s.empty() || cutset.empty()) {
    return s;
  }
  if (cutset.size() == 1 && utf8::to_rune(cutset[0]) < utf8::rune_self) {
    return detail::trim_left(detail::trim_right(s, cutset[0]), cutset[0]);
  }
  if (auto [as, ok] = detail::make_ascii_set(cutset); ok) {
    return detail::trim_left(detail::trim_right(s, as), as);
  }
  return detail::trim_left(detail::trim_right(s, cutset), cutset);
}

constexpr auto trim_left(std::string_view s, std::string_view cutset) -> std::string_view {
  namespace utf8 = unicode::utf8;
  if (s.empty() || cutset.empty()) {
    return s;
  }
  if (cutset.size() == 1 && utf8::to_rune(cutset[0]) < utf8::rune_self) {
    return detail::trim_left(s, cutset[0]);
  }
  if (auto [as, ok] = detail::make_ascii_set(cutset); ok) {
    return detail::trim_left(s, as);
  }
  return detail::trim_left(s, cutset);
}

template <typename T> requires IndexFunction<T>
constexpr auto trim_left(std::string_view s, T&& func) -> std::string_view {
  auto i = detail::index(s, std::move(func), false);
  if (i == std::string_view::npos) {
    return "";
  }
  return s.substr(i);
}

constexpr auto trim_right(std::string_view s, std::string_view cutset) -> std::string_view {
  namespace utf8 = unicode::utf8;
  if (s.empty() || cutset.empty()) {
    return s;
  }
  if (cutset.size() == 1 && utf8::to_rune(cutset[0]) < utf8::rune_self) {
    return detail::trim_right(s, cutset[0]);
  }
  if (auto [as, ok] = detail::make_ascii_set(cutset); ok) {
    return detail::trim_right(s, as);
  }
  return detail::trim_right(s, cutset);
}

template <typename T> requires IndexFunction<T>
constexpr auto trim_right(std::string_view s, T&& func) -> std::string_view {
  namespace utf8 = unicode::utf8;
  auto i = detail::last_index(s, std::move(func), false);
  if (i != std::string_view::npos && utf8::to_rune(s[i]) >= utf8::rune_self) {
    auto [_, wid] = utf8::decode(s.substr(i));
    i += wid;
  } else {
    ++i;
  }
  return s.substr(0, i);
}

constexpr auto trim_prefix(std::string_view s, std::string_view prefix) -> std::string_view {
  if (has_prefix(s, prefix)) {
    return s.substr(prefix.size());
  }
  return s;
}

constexpr auto trim_suffix(std::string_view s, std::string_view suffix) -> std::string_view {
  if (has_suffix(s, suffix)) {
    return s.substr(0, s.size()-suffix.size());
  }
  return s;
}

constexpr auto count(std::string_view s, std::string_view substr) -> long {
  namespace utf8 = unicode::utf8;
  if (substr.size() == 0) {
    return utf8::count(std::begin(s), std::end(s)) + 1;
  } else if (substr.size() == 1) {
    long n = 0;
    for (auto c : s) {
      if (c == substr[0]) {
        ++n;
      }
    }
    return n;
  }
  long n = 0;
  for (;;) {
    auto i = index(s, substr);
    if (i == std::string_view::npos) {
      return n;
    }
    ++n;
    s = s.substr(i+substr.size());
  }
}

constexpr auto has_prefix(std::string_view s, std::string_view prefix) -> bool {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

constexpr auto has_suffix(std::string_view s, std::string_view suffix) -> bool {
  return s.size() >= suffix.size() && s.substr(s.size()-suffix.size()) == suffix;
}

template <typename T> requires MapFunction<T>
auto map(T&& mapping, std::string_view s) -> std::string {
  namespace utf8 = unicode::utf8;
  auto b = builder{};

  for (auto [i, c] : utf8::range{s}) {
    auto r = mapping(c);
    if (r == c && c != utf8::rune_error) {
      continue;
    }

    long width = 0;
    if (c == utf8::rune_error) {
      std::tie(c, width) = utf8::decode(s.substr(i));
      if (width != 1 && r == c) {
        continue;
      }
    } else {
      width = utf8::len(c);
    }

    b.grow(s.size() + utf8::utf_max);
    b.write_string(s.substr(0, i));
    if (r >= 0) {
      b.write_rune(r);
    }

    s = s.substr(i+width);
    break;
  }

  if (b.capacity() == 0) {
    return std::string{s};
  }

  for (auto [_, c] : utf8::range{s}) {
    auto r = mapping(c);

    if (r >= 0) {
      if (r < utf8::rune_self) {
        b.write_byte(static_cast<uint8_t>(r));
      } else {
        b.write_rune(r);
      }
    }
  }

  return std::string{b.str()};
}

constexpr auto cut(std::string_view s, std::string_view sep) -> std::tuple<std::string_view, std::string_view, bool> {
  if (auto i = index(s, sep); i != std::string_view::npos) {
    return {s.substr(0, i), s.substr(i+sep.size()), true};
  }
  return {s, "", false};
}

}  // namespace bongo::strings
