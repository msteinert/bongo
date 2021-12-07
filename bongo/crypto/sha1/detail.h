// Copyright The Go Authors.

#pragma once

namespace bongo::crypto::sha1::detail {

constexpr static uint32_t const chunk = 64;
constexpr static uint32_t const init0 = 0x67452301;
constexpr static uint32_t const init1 = 0xefcdab89;
constexpr static uint32_t const init2 = 0x98badcfe;
constexpr static uint32_t const init3 = 0x10325476;
constexpr static uint32_t const init4 = 0xc3d2e1f0;

}  // namespace bongo::crypto::detail
