// Copyright The Go Authors.

// Package sha1 implements the SHA-1 hash algorithm as defined in RFC 3174.
//
// SHA-1 is cryptographically broken and should not be used for secure
// applications.
#pragma once

#include <span>
#include <system_error>
#include <vector>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/crypto/sha1/detail.h>

namespace bongo::crypto::sha1 {

constexpr static int const size = 20;
constexpr static int const block_size = 64;

class hash {
  uint32_t h_[5] = {detail::init0, detail::init1, detail::init2, detail::init3, detail::init4};
  uint8_t x_[detail::chunk];
  int nx_ = 0;
  uint64_t len_ = 0;

 public:
  hash() = default;
  size_t size() const noexcept;
  size_t block_size() const noexcept;
  void reset() noexcept;
  std::pair<int, std::error_code> write(std::span<uint8_t const> p);

  std::vector<uint8_t> sum();
  std::span<uint8_t> sum(std::span<uint8_t> digest);

  std::vector<uint8_t> constant_time_sum();
  std::span<uint8_t> constant_time_sum(std::span<uint8_t> digest);

  std::pair<std::vector<uint8_t>, std::error_code> marshal_binary() const;
  std::error_code unmarshal_binary(std::span<uint8_t const> b);

 private:
  std::span<uint8_t> check_sum(std::span<uint8_t> digest);
  std::span<uint8_t> const_sum(std::span<uint8_t> digest);
  void block(std::span<uint8_t const> p);
};

std::vector<uint8_t> sum(std::span<uint8_t const> data);

}  // namespace bongo::crypto
