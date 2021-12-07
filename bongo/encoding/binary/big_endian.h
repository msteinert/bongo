// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <span>

namespace bongo::encoding::binary {

struct big_endian_impl {
  template <std::output_iterator<uint8_t> OutputIt>
  constexpr OutputIt put(uint16_t v, OutputIt out) const noexcept {
    *out++ = static_cast<uint8_t>(v >> 8);
    *out++ = static_cast<uint8_t>(v);
    return out;
  }
  template <std::output_iterator<uint8_t> OutputIt>
  constexpr OutputIt put(uint32_t v, OutputIt out) const noexcept {
    *out++ = static_cast<uint8_t>(v >> 24);
    *out++ = static_cast<uint8_t>(v >> 16);
    *out++ = static_cast<uint8_t>(v >> 8);
    *out++ = static_cast<uint8_t>(v);
    return out;
  }
  template <std::output_iterator<uint8_t> OutputIt>
  constexpr OutputIt put(uint64_t v, OutputIt out) const noexcept {
    *out++ = static_cast<uint8_t>(v >> 56);
    *out++ = static_cast<uint8_t>(v >> 48);
    *out++ = static_cast<uint8_t>(v >> 40);
    *out++ = static_cast<uint8_t>(v >> 32);
    *out++ = static_cast<uint8_t>(v >> 24);
    *out++ = static_cast<uint8_t>(v >> 16);
    *out++ = static_cast<uint8_t>(v >> 8);
    *out++ = static_cast<uint8_t>(v);
    return out;
  }
  constexpr void put(std::span<uint8_t> b, uint16_t v) const noexcept {
    put(v, b.begin());
  }
  constexpr void put(std::span<uint8_t> b, uint32_t v) const noexcept {
    put(v, b.begin());
  }
  constexpr void put(std::span<uint8_t> b, uint64_t v) const noexcept {
    put(v, b.begin());
  }
  constexpr uint16_t uint16(std::span<uint8_t const> b) const noexcept {
    return static_cast<uint16_t>(b[1]) | (static_cast<uint16_t>(b[0])<<8);
  }
  constexpr uint32_t uint32(std::span<uint8_t const> b) const noexcept {
    return static_cast<uint32_t>(b[3]) | (static_cast<uint32_t>(b[2])<<8) |
      (static_cast<uint32_t>(b[1])<<16) | (static_cast<uint32_t>(b[0])<<24);
  }
  constexpr uint64_t uint64(std::span<uint8_t const> b) const noexcept {
    return static_cast<uint64_t>(b[7]) | (static_cast<uint64_t>(b[6])<<8) |
      (static_cast<uint64_t>(b[5])<<16) | (static_cast<uint64_t>(b[4])<<24) |
      (static_cast<uint64_t>(b[3])<<32) | (static_cast<uint64_t>(b[2])<<40) |
      (static_cast<uint64_t>(b[1])<<48) | (static_cast<uint64_t>(b[0])<<56);
  }
};

static const big_endian_impl big_endian;

}  // namespace bongo::encoding::binary
