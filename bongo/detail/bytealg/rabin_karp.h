// Copyright The Go Authors.

#pragma once

#include <string_view>
#include <utility>

namespace bongo::detail::bytealg {

static constexpr int prime_rk = 16777619;

// Returns the hash and the appropriate multiplicative factor for use in the
// Rabin-Karp algorithm.
constexpr auto hash(std::string_view sep) -> std::pair<uint32_t, uint32_t> {
  uint32_t h = 0;
  for (auto c : sep) {
    h = h*prime_rk + static_cast<uint32_t>(c);
  }
  uint32_t pow = 1, sq = prime_rk;
  for (auto i = sep.size(); i > 0; i >>= 1) {
    if ((i&1) != 0) {
      pow *= sq;
    }
    sq *= sq;
  }
  return {h, pow};
}

constexpr auto hash_reverse(std::string_view sep) -> std::pair<uint32_t, uint32_t> {
  uint32_t h = 0;
  for (auto i = static_cast<int>(sep.size()) - 1; i >= 0; --i) {
    h = h*prime_rk + static_cast<uint32_t>(sep[i]);
  }
  uint32_t pow = 1, sq = prime_rk;
  for (auto i = sep.size(); i > 0; i >>= 1) {
    if ((i&1) != 0) {
      pow *= sq;
    }
    sq *= sq;
  }
  return {h, pow};
}

// Uses the Rabin-Karp search algorithm to return the index of the first
// occurrence of substr in s, or npos if not present.
constexpr auto index_rabin_karp(std::string_view s, std::string_view substr) -> std::string_view::size_type {
  auto [hashss, pow] = hash(substr);
  uint32_t h = 0;
  for (std::string_view::size_type i = 0; i < substr.size(); ++i) {
    h = h*prime_rk + static_cast<uint32_t>(s[i]);
  }
  if (h == hashss && s.substr(0, substr.size()) == substr) {
    return 0;
  }
  for (auto i = substr.size(); i < s.size();) {
    h = h*prime_rk + static_cast<uint32_t>(s[i]);
    h -= pow * static_cast<uint32_t>(s[i-substr.size()]);
    ++i;
    if (h == hashss && s.substr(i-substr.size(), substr.size()) == substr) {
      return i - substr.size();
    }
  }
  return std::string_view::npos;
}

}  // namespace bongo::detail::bytealg
