// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <bongo/strings/builder.h>
#include <bongo/strings/strings.h>
#include <bongo/unicode/utf8.h>

#include <iostream>
#include <bongo/fmt.h>

namespace bongo::strings {
namespace detail {

template <std::input_iterator InputIt>
auto explode(InputIt it, InputIt end, int n) -> std::vector<std::string_view> {
  namespace utf8 = unicode::utf8;
  auto l = static_cast<int>(utf8::count(it, end));
  if (n < 0 || n > l) {
    n = l;
  }
  std::vector<std::string_view> a;
  a.reserve(n);
  while (it != end) {
    if (a.size() == static_cast<std::string_view::size_type>(n-1)) {
      a.push_back(std::string_view{it, end});
      break;
    }
    auto [_, size] = utf8::decode(it, end);
    a.push_back(std::string_view{it, size});
    it = std::next(it, size);
  }
  return a;
}

inline auto generic_split(std::string_view s, std::string_view sep, int sep_save, int n) -> std::vector<std::string_view> {
  if (n == 0) {
    return {};
  }
  if (sep == "") {
    return explode(std::begin(s), std::end(s), n);
  }
  if (n < 0) {
    n = count(s, sep) + 1;
  }
  if (static_cast<std::string_view::size_type>(n) > s.size()+1) {
    n = s.size() + 1;
  }
  std::vector<std::string_view> a;
  a.reserve(n);
  --n;
  int i = 0;
  while (i < n) {
    auto m = index(s, sep);
    if (m == std::string_view::npos) {
      break;
    }
    a.push_back(s.substr(0, m+sep_save));
    s = s.substr(m+sep.size());
    ++i;
  }
  a.push_back(s);
  return a;
}

}  // namespace detail

inline auto split_n(std::string_view s, std::string_view sep, int n) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, 0, n);
}

inline auto split_after_n(std::string_view s, std::string_view sep, int n) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, sep.size(), n);
}

inline auto split(std::string_view s, std::string_view sep) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, 0, -1);
}

inline auto split_after(std::string_view s, std::string_view sep) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, sep.size(), -1);
}

inline auto join(std::span<std::string_view> e, std::string_view sep) -> std::string {
  switch (e.size()) {
  case 0:
    return "";
  case 1:
    return std::string{e.front()};
  }
  auto n = sep.size() * (e.size() - 1);
  for (auto s : e) {
    n += s.size();
  }
  auto b = builder{};
  b.grow(n);
  auto it = std::begin(e);
  b.write_string(*it);
  for (it = std::next(it); it != std::end(e); ++it) {
    b.write_string(sep);
    b.write_string(*it);
  }
  return std::string{b.str()};
}

}  // namesapce bongo::strings
