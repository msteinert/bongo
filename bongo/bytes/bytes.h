// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <compare>
#include <iterator>
#include <span>
#include <vector>

namespace bongo::bytes {

static constexpr size_t npos = static_cast<size_t>(-1);

// index returns the index of the first instance of c in b, or npos if c is not
// present in b.
constexpr auto index(std::span<uint8_t const> b, uint8_t c) -> size_t {
  for (size_t i = 0; i < b.size(); ++i) {
    if (b[i] == c) {
      return i;
    }
  }
  return npos;
}

// compare compares two spans of bytes lexicographically.
constexpr std::strong_ordering compare(std::span<uint8_t const> a, std::span<uint8_t const> b) noexcept {
  auto l = a.size();
  if (b.size() < l) {
    l = b.size();
  }
  if (l != 0 && a.data() != b.data()) {
    for (size_t i = 0; i < l; ++i) {
      if (a[i] < b[i]) {
        return std::strong_ordering::less;
      }
      if (a[i] > b[i]) {
        return std::strong_ordering::greater;
      }
    }
  }
  if (a.size() < b.size()) {
    return std::strong_ordering::less;
  }
  if (a.size() > b.size()) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

// less reports if a is lexicographically lesser than b.
constexpr bool less(std::span<uint8_t const> a, std::span<uint8_t const> b) {
  return compare(a, b) == std::strong_ordering::less;
}

// less reports if a is lexicographically equal to b.
constexpr bool equal(std::span<uint8_t const> a, std::span<uint8_t const> b) noexcept {
  return compare(a, b) == std::strong_ordering::equal;
}

// less reports if a is lexicographically greater than b.
constexpr bool greater(std::span<uint8_t const> a, std::span<uint8_t const> b) noexcept {
  return compare(a, b) == std::strong_ordering::greater;
}

// join concatenates the elements of s to create a new std::vector<uint8_t>.
// The separator sep is placed between elements in the resulting slice.
inline std::vector<uint8_t> join(std::span<std::span<uint8_t const>> s, std::span<uint8_t const> sep = {}) {
  if (s.size() == 0) {
    return std::vector<uint8_t>{};
  }
  if (s.size() == 1) {
    return std::vector<uint8_t>{s.front().begin(), s.front().end()};
  }
  auto n = sep.size() * (s.size() - 1);
  for (auto v : s) {
    n += v.size();
  }
  auto b = std::vector<uint8_t>{};
  b.reserve(n);
  b.insert(b.end(), s.front().begin(), s.front().end());
  for (auto v : s.subspan(1)) {
    b.insert(b.end(), sep.begin(), sep.end());
    b.insert(b.end(), v.begin(), v.end());
  }
  return b;
}

// repeat returns a new std::vector<uint8_t> consisting of count copies of b.
inline std::vector<uint8_t> repeat(std::span<uint8_t const> b, size_t count) {
  if (count == 0) {
    return std::vector<uint8_t>{};
  }
  auto nb = std::vector<uint8_t>{};
  nb.reserve(b.size()*count);
  for (size_t i = 0; i < count; ++i) {
    nb.insert(nb.end(), b.begin(), b.end());
  }
  return nb;
}

// to_string creates a std::string_view from contiguous container.
template <typename T>
std::string_view to_string(T&& s) {
  return std::string_view{reinterpret_cast<char const*>(std::data(s)), std::size(s)};
}

// to_string creates a std::string_view from contiguous container.
template <typename T>
std::string_view to_string(T&& s, long size) {
  return std::string_view{reinterpret_cast<char const*>(std::data(s)), static_cast<size_t>(size)};
}

// to_bytes creates a std::span<uint8_t> from contiguous container.
template <typename T>
std::span<uint8_t const> to_bytes(T&& s) {
  return std::span{reinterpret_cast<uint8_t const*>(std::data(s)), std::size(s)};
}

// to_bytes creates a std::span<uint8_t> from contiguous container.
template <typename T>
std::span<uint8_t const> to_bytes(T&& s, long size) {
  return std::span{reinterpret_cast<uint8_t const*>(std::data(s)), static_cast<size_t>(size)};
}

}  // namespace bongo::bytes
