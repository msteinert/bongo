// Copyright The Go Authors.

#include <string>
#include <string_view>

#include <bongo/strings/builder.h>
#include <bongo/strings/strings.h>
#include <bongo/unicode/utf8.h>

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

auto generic_split(std::string_view s, std::string_view sep, int sep_save, int n) -> std::vector<std::string_view> {
  if (n == 0) {
    return {};
  }
  if (sep.empty()) {
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

auto split(std::string_view s, std::string_view sep, int n) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, 0, n);
}

auto split_after(std::string_view s, std::string_view sep, int n) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, sep.size(), n);
}

auto split(std::string_view s, std::string_view sep) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, 0, -1);
}

auto split_after(std::string_view s, std::string_view sep) -> std::vector<std::string_view> {
  return detail::generic_split(s, sep, sep.size(), -1);
}

auto join(std::span<std::string_view> e, std::string_view sep) -> std::string {
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

auto to_valid_utf8(std::string_view s, std::string_view replacement) -> std::string {
  namespace utf8 = unicode::utf8;
  auto b = builder{};

  for (auto [i, c] : utf8::range{s}) {
    if (c != utf8::rune_error) {
      continue;
    }

    auto [_, wid] = utf8::decode(s.substr(i));
    if (wid == 1) {
      b.grow(s.size() + replacement.size());
      b.write_string(s.substr(0, i));
      s = s.substr(i);
      break;
    }
  }

  if (b.capacity() == 0) {
    return std::string{s};
  }

  auto invalid = false;
  for (std::string_view::size_type i = 0; i < s.size();) {
    auto c = static_cast<uint8_t>(s[i]);
    if (c < utf8::rune_self) {
      ++i;
      invalid = false;
      b.write_byte(c);
      continue;
    }
    auto [_, wid] = utf8::decode(s.substr(i));
    if (wid == 1) {
      ++i;
      if (!invalid) {
        invalid = true;
        b.write_string(replacement);
      }
      continue;
    }
    invalid = false;
    b.write_string(s.substr(i, wid));
    i += wid;
  }

  return std::string{b.str()};
}

auto repeat(std::string_view s, size_t count) -> std::string {
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

auto replace(std::string_view s, std::string_view old_s, std::string_view new_s, int n) -> std::string {
  namespace utf8 = unicode::utf8;
  if (old_s == new_s || n == 0) {
    return std::string{s};
  }
  if (auto m = count(s, old_s); m == 0) {
    return std::string{s};
  } else if (n < 0 || m < n) {
    n = m;
  }
  auto b = builder{};
  b.grow(s.size() + n*(new_s.size()-old_s.size()));
  auto start = 0;
  for (auto i = 0; i < n; ++i) {
    auto j = start;
    if (old_s.size() == 0) {
      if (i > 0) {
        auto [_, wid] = utf8::decode(s.substr(start));
        j += wid;
      }
    } else {
      j += index(s.substr(start), old_s);
    }
    b.write_string(s.substr(start, j-start));
    b.write_string(new_s);
    start = j + old_s.size();
  }
  b.write_string(s.substr(start));
  return std::string{b.str()};
}

auto replace(std::string_view s, std::string_view old_s, std::string_view new_s) -> std::string {
  return replace(s, old_s, new_s, -1);
}

}  // namesapce bongo::strings
